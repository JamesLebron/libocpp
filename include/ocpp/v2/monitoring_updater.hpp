// SPDX-License-Identifier: Apache-2.0
// Copyright 2020 - 2024 Pionix GmbH and Contributors to EVerest

#pragma once

#include <unordered_map>

#include <everest/timer.hpp>

#include <ocpp/v2/enums.hpp>
#include <ocpp/v2/ocpp_enums.hpp>
#include <ocpp/v2/ocpp_types.hpp>

#include <ocpp/v2/device_model_storage_interface.hpp>

namespace ocpp::v2 {

/**
 * @brief 前置申明DeviceModel类
 *
 */
class DeviceModel;

/**
 * @brief 定义更新监控元数据类型
 *
 * @enum TRIGGER 触发类型
 * @enum PERIODIC 周期类型
 *
 */
enum UpdateMonitorMetaType {
    TRIGGER,
    PERIODIC
};

/**
 * @brief 定义触发监控元数据类型
 *
 */
struct TriggerMetadata {
    /// @brief 如果我们至少向 CSMS 发送过一次触发事件，那么我们只有在将清除状态发送给 CSMS 之后，才会清除该触发器。
    std::uint32_t is_csms_sent_triggered : 1;

    /// @brief 该事件是针对当前状态生成的，每次状态变化时会重置。
    std::uint32_t is_event_generated : 1;

    /// @brief 如果当前状态已发送给 CSMS，则每次状态变化时会重置。
    std::uint32_t is_csms_sent : 1;

    /// \brief 触发器已被清除，即在检测到问题后恢复到正常状态。
    /// 当触发器被清除时，可以从映射中移除，但前提是必须先将其发送给 CSMS，
    /// 且仅在之前的触发事件确实已发送给 CSMS 的情况下。如果清除仅发生在内部状态中，
    /// 则可以直接从映射中移除。
    std::uint32_t is_cleared : 1;
};

/**
 * @brief 定义周期监控元数据类型
 *
 */
struct PeriodicMetadata {
    /// @brief 上次触发时间
    std::chrono::time_point<std::chrono::steady_clock> last_trigger_steady;

    /// \brief 下次需要触发与时钟对齐的值时使用，仅对周期监控器有意义。
    std::chrono::time_point<std::chrono::system_clock> next_trigger_clock_aligned;
};

/// \brief 用于我们内部保存需求的元数据
struct UpdaterMonitorMeta {
    /// @brief 元数据类型  TRIGGER 或 PERIODIC
    UpdateMonitorMetaType type;

    /// @brief 变量监控元数据
    VariableMonitoringMeta monitor_meta;

    /// @brief 组件类型
    Component component;

    /// @brief 变量类型
    Variable variable;

    /// @brief 数据库 ID，用于在需要时快速即时检索。
    std::int32_t monitor_id;

    /// @brief 上次的值
    std::string value_previous;

    /// @brief 当前值
    std::string value_current;

    /// @brief 只写值不会被上报。
    std::uint32_t is_writeonly : 1;

    /// @brief 触发监控元数据
    TriggerMetadata meta_trigger;

    /// @brief 周期监控元数据
    PeriodicMetadata meta_periodic;

    /// \brief Generated monitor events, that are related to this meta
    std::vector<EventData> generated_monitor_events;

public:
    /// @brief Can trigger/clear an event ？？？   仅触发监控元数据才需要调用
    void set_trigger_clear_state(bool is_cleared) {
        if (type != UpdateMonitorMetaType::TRIGGER) {
            throw std::runtime_error("Clear state should never be used on a non-trigger meta!");
        }

        if (meta_trigger.is_cleared != is_cleared) {
            meta_trigger.is_cleared = is_cleared;

            // On a state change reset the CSMS sent status and
            // event generation status
            meta_trigger.is_csms_sent = 0;
            meta_trigger.is_event_generated = 0;
        }
    }
};

/// @brief 定义发送通知的回调
typedef std::function<void(const std::vector<EventData>&)> notify_events;

/// @brief 定义检查是否离线的回调
typedef std::function<bool()> is_offline;

class MonitoringUpdater {

public:
    /// @brief 禁用默认构造函数
    MonitoringUpdater() = delete;

    /// \brief 构造一个新的变量监控更新器
    /// \param device_model 当前使用的变量设备模型
    /// \param notify_csms_events 可用于触发若干告警事件的函数
    /// \param is_chargepoint_offline 可用于获取充电站与 CSMS 连接状态的函数
    MonitoringUpdater(DeviceModel& device_model, notify_events notify_csms_events, is_offline is_chargepoint_offline);
    ~MonitoringUpdater();

public:
    /// \brief 启动变量监控，启动定时器
    void start_monitoring();
    /// \brief 停止变量监控，取消定时器
    void stop_monitoring();

    /// \brief
    /// 处理由变量触发的监控器。
    /// 在相关变量修改操作之后会被调用，或者在当前无法立即处理时，会周期性调用，
    /// 例如在内部变量被修改的情况下。
    void process_triggered_monitors();

private:
    /// \brief 注册到 'device_model' 的回调函数，用于判断在使用内部值时，某个变量是否触发了监控器。
    /// 会延迟将监控器发送给 CSMS，直到充电站完成当前操作。
    /// 原因是变量可能在操作过程中发生变化，而 CSMS 并不期望接收类型为 'EventData' 的消息，
    /// 因此处理会被延迟，直到手动调用 'process_triggered_monitors' 或周期性监控定时器触发。
    /// \param monitors 监控列表
    /// \param component 组件
    /// \param variable 变量
    /// \param characteristics 变量特征
    /// \param attribute 变量属性
    /// \param value_old 旧值
    /// \param value_current 当前值
    void on_variable_changed(const std::unordered_map<int64_t, VariableMonitoringMeta>& monitors,
                             const Component& component, const Variable& variable,
                             const VariableCharacteristics& characteristics, const VariableAttribute& attribute,
                             const std::string& value_old, const std::string& value_current);

    /// \brief 注册到 'device_model' 的回调函数，用于判断已有监控器是否被更新。
    /// 对于某些规范要求，在监控器更新时必须刷新监控器数据，这个回调是必需的。
    /// \param updated_monitor 更新后的监控器
    /// \param component 组件
    /// \param variable 变量
    /// \param characteristics 变量特征
    /// \param attribute 变量属性
    /// \param current_value 当前值
    void on_monitor_updated(const VariableMonitoringMeta& updated_monitor, const Component& component,
                            const Variable& variable, const VariableCharacteristics& characteristics,
                            const VariableAttribute& attribute, const std::string& current_value);

    /// \brief 判断监控器是否被触发，如果被触发，则将其加入我们的内部列表。
    /// \param monitor_meta 监控器元数据
    /// \param component 组件
    /// \param variable 变量
    /// \param characteristics 变量特征
    /// \param attribute 变量属性
    /// \param value_previous 旧值
    /// \param value_current 当前值
    void evaluate_monitor(const VariableMonitoringMeta& monitor_meta, const Component& component,
                          const Variable& variable, const VariableCharacteristics& characteristics,
                          const VariableAttribute& attribute, const std::string& value_previous,
                          const std::string& value_current);

    /// \brief 处理周期性监控器。由于这可能是一个相对耗时的操作（需要查询每个已触发监控器的实际值），
    /// 可以通过内部变量 'VariableMonitoringProcessTime' 配置处理时间。
    /// 如果还有任何待处理的告警触发监控器，也会一并处理。
    /// \param allow_periodics 是否允许处理周期性监控器
    /// \param allow_trigger 是否允许处理触发监控器
    void process_monitors_internal(bool allow_periodics, bool allow_trigger);

    /// \brief 处理监控器元数据，在其内部列表中生成所有所需事件。
    /// 它将为通知生成 EventData，而不受离线状态影响。
    void process_monitor_meta_internal(UpdaterMonitorMeta& updater_meta_data);

    /// @brief 根据当前的元数据内部状态，判断是否适合将指定的监控元数据从内部列表中移除。
    /// 这意味着需要针对不同状态进行多种检查。
    bool should_remove_monitor_meta_internal(const UpdaterMonitorMeta& updater_meta_data);

    /// @brief 查询数据库（从内存数据中进行快速检索），
    /// 并使用新的数据库数据更新我们的内部监控器。
    void update_periodic_monitors_internal();

    /// @brief 获取监视器组件的配置信息
    void get_monitoring_info(bool& out_is_offline, int& out_offline_severity, int& out_active_monitoring_level,
                             MonitoringBaseEnum& out_active_monitoring_base);

    /// \brief  监视器组件是否启用
    bool is_monitoring_enabled();

private:
    /// @brief 设备模型
    DeviceModel& device_model;
    /// @brief 监视器timer
    Everest::SteadyTimer monitors_timer;
    /// @brief 充电点到CSMS消息唯一ID，用于EventData
    std::int32_t unique_id;
    /// @brief 发送监视器触发的事件通知
    notify_events notify_csms_events;
    /// @brief 检查是否离线
    is_offline is_chargepoint_offline;

    /// @brief 内部监控器元数据
    /// 键：监控器ID
    /// 值：监控器元数据
    std::unordered_map<std::int32_t, UpdaterMonitorMeta> updater_monitors_meta;
};

} // namespace ocpp::v2
