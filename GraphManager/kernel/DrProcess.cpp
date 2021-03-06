/*
Copyright (c) Microsoft Corporation

All rights reserved.

Licensed under the Apache License, Version 2.0 (the "License"); you may not use this file except in 
compliance with the License.  You may obtain a copy of the License 
at http://www.apache.org/licenses/LICENSE-2.0   


THIS CODE IS PROVIDED *AS IS* BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, EITHER 
EXPRESS OR IMPLIED, INCLUDING WITHOUT LIMITATION ANY IMPLIED WARRANTIES OR CONDITIONS OF 
TITLE, FITNESS FOR A PARTICULAR PURPOSE, MERCHANTABLITY OR NON-INFRINGEMENT.  


See the Apache Version 2.0 License for specific language governing permissions and 
limitations under the License. 

*/

#include <DrKernel.h>

DrProcessHandle::~DrProcessHandle()
{
}

void DrProcessHandle::SetAssignedNode(DrResourcePtr node)
{
    m_node = node;
}

DrResourcePtr DrProcessHandle::GetAssignedNode()
{
    return m_node;
}


DrProcessTemplate::DrProcessTemplate()
{
    m_failedRetainTime = 0;
    m_failedLeaseGraceTime = 0;
    m_completedRetainTime = 0;
    m_completedLeaseGraceTime = 0;
    m_maxMemory = MAX_UINT64;
    m_timeOutBetweenProcessEndAndVertexNotification = 0;
    m_affinityLevelThresholds = DrNew DrFloatArray(DRL_Cluster + 1);
    int i;
    for (i=0; i<=DRL_Cluster; ++i)
    {
        /* default threshold is keep all affinities */
        m_affinityLevelThresholds[i] = 0.0f;
    }
    m_listenerList = DrNew DrProcessListenerIRefList();
}

void DrProcessTemplate::SetCommandLineBase(DrString commandLine)
{
    m_commandLineBase = commandLine;
}

DrString DrProcessTemplate::GetCommandLineBase()
{
    return m_commandLineBase;
}

DrProcessListenerIRefListPtr DrProcessTemplate::GetListenerList()
{
    return m_listenerList;
}

void DrProcessTemplate::SetProcessClass(DrString processClass)
{
    m_processClass = processClass;
}

DrString DrProcessTemplate::GetProcessClass()
{
    return m_processClass;
}

void DrProcessTemplate::SetFailedRetainAndLeaseGraceTime(DrTimeInterval retainTime,
                                                         DrTimeInterval leaseGraceTime)
{
    DrAssert(retainTime > leaseGraceTime);
    m_failedRetainTime = retainTime;
    m_failedLeaseGraceTime = leaseGraceTime;
}

DrTimeInterval DrProcessTemplate::GetFailedRetainTime()
{
    return m_failedRetainTime;
}

DrTimeInterval DrProcessTemplate::GetFailedLeaseWaitTime()
{
    return m_failedRetainTime - m_failedLeaseGraceTime;
}

void DrProcessTemplate::SetCompletedRetainAndLeaseGraceTime(DrTimeInterval retainTime,
                                                            DrTimeInterval leaseGraceTime)
{
    DrAssert(retainTime > leaseGraceTime);
    m_completedRetainTime = retainTime;
    m_completedLeaseGraceTime = leaseGraceTime;
}

DrTimeInterval DrProcessTemplate::GetCompletedRetainTime()
{
    return m_completedRetainTime;
}

DrTimeInterval DrProcessTemplate::GetCompletedLeaseWaitTime()
{
    return m_completedRetainTime - m_completedLeaseGraceTime;
}

void DrProcessTemplate::SetMaxMemory(UINT64 maxMemory)
{
    m_maxMemory = maxMemory;
}

UINT64 DrProcessTemplate::GetMaxMemory()
{
    return m_maxMemory;
}

void DrProcessTemplate::SetTimeOutBetweenProcessEndAndVertexNotification(DrTimeInterval timeOut)
{
    m_timeOutBetweenProcessEndAndVertexNotification = timeOut;
}

DrTimeInterval DrProcessTemplate::GetTimeOutBetweenProcessEndAndVertexNotification()
{
    return m_timeOutBetweenProcessEndAndVertexNotification;
}

DrFloatArrayPtr DrProcessTemplate::GetAffinityLevelThresholds()
{
    return m_affinityLevelThresholds;
}


DrProcessStateRecord::DrProcessStateRecord()
{
    m_state = DPS_NotStarted;
}

DrProcessStateRecordRef DrProcessStateRecord::Clone()
{
    DrProcessStateRecordRef r = DrNew DrProcessStateRecord();

    r->m_state = m_state;
    r->m_process = m_process;
    r->m_exitCode = m_exitCode;
    r->m_status = m_status;

    return r;
}


DrProcessStats::DrProcessStats()
{
    m_exitCode = STILL_ACTIVE;
    m_pid = 0;
    m_createdTime = DrDateTime_Never;
    m_beginExecutionTime = DrDateTime_Never;
    m_terminatedTime = DrDateTime_Never;
    m_userTime = 0;
    m_kernelTime = 0;
    m_pageFaults = 0;
    m_peakVMUsage = 0;
    m_peakMemUsage = 0;
    m_memUsageSeconds = 0;
    m_totalIO = 0;
}

bool DrProcessStats::Different(DrProcessStatsPtr other)
{
    return
        m_exitCode != other->m_exitCode ||
        m_pid != other->m_pid ||
        m_createdTime != other->m_createdTime ||
        m_beginExecutionTime != other->m_beginExecutionTime ||
        m_terminatedTime != other->m_terminatedTime ||
        m_userTime != other->m_userTime ||
        m_kernelTime != other->m_kernelTime ||
        m_pageFaults != other->m_pageFaults ||
        m_peakVMUsage != other->m_peakMemUsage ||
        m_peakMemUsage != other->m_peakMemUsage ||
        m_memUsageSeconds != other->m_memUsageSeconds ||
        m_totalIO != other->m_totalIO;
}

DrProcessStatsRef DrProcessStats::Clone()
{
    DrProcessStatsRef ps = DrNew DrProcessStats();

    ps->m_exitCode = m_exitCode;
    ps->m_pid = m_pid;
    ps->m_createdTime = m_createdTime;
    ps->m_beginExecutionTime = m_beginExecutionTime;
    ps->m_terminatedTime = m_terminatedTime;
    ps->m_userTime = m_userTime;
    ps->m_kernelTime = m_kernelTime;
    ps->m_pageFaults = m_pageFaults;
    ps->m_peakVMUsage = m_peakVMUsage;
    ps->m_peakMemUsage = m_peakMemUsage;
    ps->m_memUsageSeconds = m_memUsageSeconds;
    ps->m_totalIO = m_totalIO;

    return ps;
}


DrPropertyStatus::DrPropertyStatus(DrProcessBasicState state, UINT32 exitCode, DrErrorPtr error)
{
    m_processState = state;
    m_exitCode = exitCode;
    m_status = error;
    m_statusVersion = 0;
}


DrProcess::DrProcess(DrXComputePtr xc, DrString name, DrString commandLine,
                     DrProcessTemplatePtr processTemplate)
    : DrNotifier<DrProcessInfoRef>(xc->GetMessagePump())
{
    m_xc = xc;
    m_name = name;
    m_commandLine = commandLine;
    m_template = processTemplate;

    m_affinity = DrNew DrAffinityList();

    m_info = DrNew DrProcessInfo();
    m_info->m_process = DrNull; /* don't create a circular reference */
    m_info->m_state = DrNew DrProcessStateRecord();
    m_info->m_statistics = DrNew DrProcessStats();

    m_info->m_jmProcessCreatedTime = m_xc->GetCurrentTimeStamp();
    m_info->m_jmProcessScheduledTime = DrDateTime_Never;

    m_hasEverRequestedProperty = false;

    DrProcessListenerIRefListRef listeners = processTemplate->GetListenerList();
    int i;
    for (i=0; i<listeners->Size(); ++i)
    {
        AddListener(listeners[i]);
    }
}

void DrProcess::SetAffinityList(DrAffinityListPtr list)
{
    m_affinity = list;
}

DrAffinityListPtr DrProcess::GetAffinityList()
{
    return m_affinity;
}

DrString DrProcess::GetName()
{
    return m_name;
}

DrProcessInfoPtr DrProcess::GetInfo()
{
    return m_info;
}

void DrProcess::CloneAndDeliverNotification(bool delay)
{
    DrProcessInfoRef info = DrNew DrProcessInfo();

    info->m_process = this;
    info->m_state = m_info->m_state->Clone();
    info->m_statistics = m_info->m_statistics->Clone();
    info->m_jmProcessCreatedTime = m_info->m_jmProcessCreatedTime;
    info->m_jmProcessScheduledTime = m_info->m_jmProcessScheduledTime;

    if (delay)
    {
        DeliverDelayedNotification(m_template->GetTimeOutBetweenProcessEndAndVertexNotification(), info);
    }
    else
    {
        DeliverNotification(info);
    }
}

void DrProcess::Schedule()
{
    DrAssert(m_info->m_state->m_state == DPS_NotStarted);

    m_info->m_state->m_state = DPS_Initializing;
    m_info->m_jmProcessScheduledTime = m_xc->GetCurrentTimeStamp();

    m_xc->ScheduleProcess(m_affinity, m_name, m_commandLine, m_template, this);
}

void DrProcess::RequestProperty(UINT64 lastSeenVersion, DrString propertyName,
                                DrTimeInterval maxBlockTime, DrPropertyListenerPtr listener)
{
    DrAssert(m_info->m_state->m_state > DPS_Scheduling);

    if (m_info->m_state->m_state < DPS_Zombie)
    {
        DrAssert(m_info->m_state->m_process != DrNull);
        /* once the higher level has ever requested a property we are going to assume it continues to do so.
           this will delay raw process end messages to the listeners to try to order them after the end
           messages that return with property fetches */
        m_hasEverRequestedProperty = true;

        m_xc->GetProcessProperty(m_info->m_state->m_process, lastSeenVersion, propertyName,
                                 maxBlockTime, this, listener);
    }
    else
    {
        /* there's no process to get the property from any more, so send a dummy dead version */
        DrString reason = "Process already in zombie state";
        DrErrorRef error = DrNew DrError(DrError_Unexpected, "DrProcess", reason);
        DrPropertyStatusRef status = DrNew DrPropertyStatus(DPBS_Failed, 1, error);
        DrPropertyMessageRef message = DrNew DrPropertyMessage(listener, status);
        DeliverMessage(message);
    }
}

void DrProcess::SendCommand(UINT64 newVersion, DrString propertyName,
                            DrString propertyDescription, DrByteArrayPtr propertyBlock)
{
    DrAssert(m_info->m_state->m_state > DPS_Scheduling);

    if (m_info->m_state->m_state < DPS_Failed)
    {
        DrAssert(m_info->m_state->m_process != DrNull);
        m_xc->SetProcessCommand(m_info->m_state->m_process, newVersion, propertyName,
                                propertyDescription, propertyBlock, this);
    }
}

void DrProcess::Terminate()
{
    if (m_info->m_state->m_process == DrNull)
    {
        /* this should be an extremely rare race: the process has been requested,
           but XCompute has not yet delivered the message with the process handle.
           We will delay another 10 seconds and then try to terminate again: we
           can't just give up, because otherwise the process, when it does get
           through the XCompute machinery, would be orphaned and would sit there
           consuming a cluster resource: we actually do want to call CancelScheduleProcess
           on it eventually */

        DrLogI("Process %s has not yet received its handle: rescheduling termination", m_name.GetChars());
        DrPStateMessageRef message = DrNew DrPStateMessage(this, DPS_Failed);
        DeliverDelayedMessage(DrTimeInterval_Second * 10, message);
        return;
    }

    DrAssert(m_info->m_state->m_state > DPS_NotStarted);

    DrAssert(m_info->m_state->m_process != DrNull);

    /* This method is also called by completed processes in order to close
       the process handle.  Don't bother cancelling already completed processes. */
    if (m_info->m_state->m_state <= DPS_Running)
    {
        m_xc->CancelScheduleProcess(m_info->m_state->m_process);
    }
    m_info->m_state->m_process->CloseHandle();
}

/* this is called whenever a state change message returns */
void DrProcess::ReceiveMessage(DrProcessStateRecordRef message)
{
    DrAssert(m_info->m_state->m_state > DPS_NotStarted);

    DrProcessState oldState = m_info->m_state->m_state;

    if (oldState == DPS_Zombie)
    {
        DrLogI("Process %s ignoring message while in zombie state", m_name.GetChars());
        /* this process is being discarded so don't do anything */
        return;
    }

    if (message->m_state < oldState)
    {
        /* this shouldn't happen */
        DrLogW("Process %s ignoring message because state is retreating old %d new %d",
               m_name.GetChars(), oldState, message->m_state);
        return;
    }

    if (m_info->m_state->m_state == DPS_Initializing)
    {
        DrAssert(m_info->m_state->m_process == DrNull);
    }
    else
    {
        DrAssert(m_info->m_state->m_process == message->m_process);
    }

    m_info->m_state = message;

    DrString errorText = DrError::ToShortText(message->m_status);
    DrLogI("Process %s in state %d message with state %d status %s",
           m_name.GetChars(), oldState, message->m_state, errorText.GetChars());

    /* make sure the listeners see an orderly state machine transition. The rule is that we go in an orderly
       sequence from Initializing=>Completed, except we can jump to Failed or Zombie at any time. From Failed
       we can only move to Zombie, and from Zombie nowhere */
    bool delayForProperty;
    DrAssert(oldState > DPS_NotStarted);
    while (m_info->m_state->m_state < DPS_Failed &&
           (int) (m_info->m_state->m_state) > ((int) oldState + 1))
    {
        DrProcessState savedState = m_info->m_state->m_state;

        /* insert the phantom intermediate state to the reported state sequence. If this update reports
           a termination and the listeners have started their property-fetch state machine then we will
           delay delivery a little to give them a chance to fetch the property reporting termination in
           more detail */
        m_info->m_state->m_state = (DrProcessState) ((int) oldState + 1);
        delayForProperty = (m_info->m_state->m_state > DPS_Running) && m_hasEverRequestedProperty;
        CloneAndDeliverNotification(delayForProperty);

        /* then fix things up for the next iteration of this loop */
        oldState = m_info->m_state->m_state;
        m_info->m_state->m_state = savedState;
    }

    /* tell everyone that is listening that something has happened. If this update reports a termination
       and the listeners have started their property-fetch state machine then we will delay delivery a
       little to give them a chance to fetch the property reporting termination in more detail */
    delayForProperty = (m_info->m_state->m_state > DPS_Running) && m_hasEverRequestedProperty;
    CloneAndDeliverNotification(delayForProperty);

    if (m_info->m_state->m_state < DPS_Starting || m_info->m_state->m_state > DPS_Running)
    {
        /* we are still waiting to get assigned somewhere or the process has finished: do nothing.
           In the future we may need to think about leases for working directories. */
        return;
    }

    if (m_info->m_state->m_state < DPS_Running)
    {
        m_xc->WaitUntilStart(m_info->m_state->m_process, this);
        return;
    }
    else
    {
        m_xc->WaitUntilCompleted(m_info->m_state->m_process, this);
    }
}

void DrProcess::ReceiveMessage(DrProcessPropertyStatusRef message)
{
    /* forward the property info to the listener that asked for it */
    DeliverMessage(message->m_message);

    if (message->m_statistics != DrNull &&
        (m_info->m_statistics == DrNull ||
         m_info->m_statistics->Different(message->m_statistics)))
    {
        /* we've got updated statistics about the process: tell the world. Since this is
           guaranteed to be delivered to the graph after the message, we don't need to
           delay (the cohort and vertex share a lock, so their deliveries are serialized) */
        m_info->m_statistics = message->m_statistics;
        CloneAndDeliverNotification(false);
    }
}

void DrProcess::ReceiveMessage(DrErrorRef message)
{
    if (message != DrNull)
    {
        if (SUCCEEDED(message->m_code))
        {
            DrString errorText = DrError::ToShortText(message);
            DrLogW("Command received non-error status %s", errorText.GetChars());
        }
        else
        {
            /* if we failed to send a message then we assume the process is in an unknown state
               and tell our listeners to abandon it */
            m_info->m_state->m_state = DPS_Zombie;
            DrString reason;
            reason.SetF("XCompute command send failed with error %s", DRERRORSTRING(message->m_code));
            m_info->m_state->m_status = DrNew DrError(DrError_XComputeError, "DrProcess", reason);
            m_info->m_state->m_status->AddProvenance(message);
            CloneAndDeliverNotification(false);
        }
    }
}

void DrProcess::ReceiveMessage(DrProcessState message)
{
    if (message == DPS_Failed)
    {
        /* This is typically the result of a vertex completing with it's cohort process still in the
           running state, which is perfectly normal. In that case, the cohort sends us a delayed message
           to terminate, so that cluster resources used by the process can be cleaned up. */
        Terminate();
    }
}