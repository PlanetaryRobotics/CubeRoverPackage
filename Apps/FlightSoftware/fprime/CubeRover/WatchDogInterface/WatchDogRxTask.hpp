#ifndef WatchDogInterface_WatchDogRxTask_HPP
#define WatchDogInterface_WatchDogRxTask_HPP

#include <Os/Task.hpp>

#include <Include/FswPacket.hpp>
#include <CubeRover/WatchDogInterface/WatchDogMpsm.hpp>

/**
 * The maximum number of classes that can register to receive callbacks from the WatchDogRxTask.
 */
#define WATCHDOG_RX_TASK__MAX_NUM_CALLBACKS 1

namespace CubeRover
{   
    /**
     * @brief Defines the contract (i.e. interface) that must be implemented (via subclassing) by classes
     *        that want to receive callbacks from the WatchDogRxTask with received messages.
     *
     * This callback will be called with each message that is received by the WatchDogRxTask instance to which
     * the object implementing this function is registered.
     */
    class WatchDogRxCallbackProcessor
    {
        public:
            /**
             * @brief The callback invoked by the `WatchDogRxTask` when it has received a message.
             *
             * @param msg The parsed message received from the MSP430 watchdog.
             * @param goodParity Whether or not `msg` passed its parity check. If false, `msg` will contain only a header.
             */
            virtual void rxCallback(WatchDogMpsm::Message& msg, bool goodParity) = 0;
    };

    /**
     * The task responsible for receiving and parsing messages from the MSP430 watchdog.
     *
     * This is a subclass of Os::Task so that it can access the `m_handle` field of Os::Task. Os::Task was modified
     * in order to make this field protected. The `m_handle` field contains the native handle of the underlying
     * implementation of Os::Task. In our case, we expect this will always be run on the Hercules, and that the
     * underlying Os::Task implementation will be the one that is implemented using FreeRTOS. We want access to this
     * because we want to use FreeRTOS Task Notifications (https://www.freertos.org/RTOS-task-notifications.html)
     * rather than FreeRTOS Semaphores in order to implement the task blocking and then being woken up by an
     * external trigger. We want to use Task Notifications over Semaphores because "Unblocking an RTOS task with
     * a direct notification is 45% faster and uses less RAM than unblocking a task using an intermediary
     * object such as a binary semaphore." (Source: https://www.freertos.org/RTOS-task-notifications.html).
     * However, in order to use Task Notifications we need access to the underlying task handle, thus the
     * need for the modifications and usage described above.
     */
    class WatchDogRxTask : public ::Os::Task
    {
        public:
            /**
             * @brief Constructor. Does not start the task.
             */
            explicit WatchDogRxTask();

            /**
             * @brief Destructor. Stops the task if it is currently running.
             */
            ~WatchDogRxTask();

            /**
             * @brief Starts the FreeRTOS task that underlies this object.
             *
             * @param priority The priority to use for the task being started.
             * @param stackSize The stack size to use for the task being started.
             * @param cpuAffinity The CPU affinity to use for the task being started, or -1 to have no affinity.
             *
             * @return The status of starting the task.
             */
            ::Os::Task::TaskStatus start(NATIVE_INT_TYPE priority,
                                         NATIVE_INT_TYPE stackSize,
                                         NATIVE_INT_TYPE cpuAffinity = -1); //!< start the task

            /**
             * @brief Registers the given object as a callback to be invoked once a message is recieved and parsed.
             *
             * @param callback The callback object to register.
             *
             * @return True if the registration succeeded, otherwise false.
             */
            bool registerCallback(WatchDogRxCallbackProcessor* callback);

        private:
            /**
             * The array containing callbacks to be invoked upon receiving and parsing a message.
             */
            WatchDogRxCallbackProcessor* m_callbacks[WATCHDOG_RX_TASK__MAX_NUM_CALLBACKS];

            /**
             * The number of indices in `m_callbacks` that contain callbacks.
             */
            size_t m_numCallbacksRegistered;

            /**
             * The message parsing state machine that informs this task how to behave in order to properly receive
             * messages from the MSP430 watchdog.
             */
            WatchDogMpsm m_mpsm;

            /**
             * Whether or not the task should keep running. The main loop in the task thread is controlled by this.
             */
            bool m_keepRunning;

            /**
             * Whether or not the task has been started. Only used to prevent calling start(...) after it has already
             * been called before.
             */
            bool m_isRunning;

            /**
             * The buffer to used for holding the payload of messages received from the MSP430 watchdog.
             */
            uint8_t m_dataBuffer[WATCHDOG_MAX_PAYLOAD] __attribute__((aligned(8)));

            /**
             * The function that implements the task thread.
             *
             * @param arg The argument to the thread, which in this case will be the `this` pointer for this object.
             */
            static void rxHandlerTaskFunction(void* arg);

            /**
             * Simply iterates through `m_callbacks` and calls all callbacks with the given parameters.
             *
             * @param msg The received message.
             * @param goodParity Whether or not `msg` passed its parity check.
             */
            void callAllCallbacks(WatchDogMpsm::Message& msg, bool goodParity);
    };


} // end namespace CubeRover

#endif // #ifndef WatchDogInterface_WatchDogRxTask_HPP
