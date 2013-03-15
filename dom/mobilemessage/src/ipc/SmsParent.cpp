/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "SmsParent.h"
#include "nsISmsService.h"
#include "nsIObserverService.h"
#include "mozilla/Services.h"
#include "Constants.h"
#include "nsIDOMMozSmsMessage.h"
#include "mozilla/unused.h"
#include "SmsMessage.h"
#include "nsIMobileMessageDatabaseService.h"
#include "SmsFilter.h"
#include "SmsSegmentInfo.h"
#include "nsIDOMDOMCursor.h"
#include "nsIDOMDOMRequest.h"
#include "MobileMessageThread.h"

namespace mozilla {
namespace dom {
namespace mobilemessage {

NS_IMPL_ISUPPORTS1(SmsParent, nsIObserver)

SmsParent::SmsParent()
{
  MOZ_COUNT_CTOR(SmsParent);
  nsCOMPtr<nsIObserverService> obs = services::GetObserverService();
  if (!obs) {
    return;
  }

  obs->AddObserver(this, kSmsReceivedObserverTopic, false);
  obs->AddObserver(this, kSmsSendingObserverTopic, false);
  obs->AddObserver(this, kSmsSentObserverTopic, false);
  obs->AddObserver(this, kSmsFailedObserverTopic, false);
  obs->AddObserver(this, kSmsDeliverySuccessObserverTopic, false);
  obs->AddObserver(this, kSmsDeliveryErrorObserverTopic, false);
}

void
SmsParent::ActorDestroy(ActorDestroyReason why)
{
  nsCOMPtr<nsIObserverService> obs = services::GetObserverService();
  if (!obs) {
    return;
  }

  obs->RemoveObserver(this, kSmsReceivedObserverTopic);
  obs->RemoveObserver(this, kSmsSendingObserverTopic);
  obs->RemoveObserver(this, kSmsSentObserverTopic);
  obs->RemoveObserver(this, kSmsFailedObserverTopic);
  obs->RemoveObserver(this, kSmsDeliverySuccessObserverTopic);
  obs->RemoveObserver(this, kSmsDeliveryErrorObserverTopic);
}

NS_IMETHODIMP
SmsParent::Observe(nsISupports* aSubject, const char* aTopic,
                   const PRUnichar* aData)
{
  if (!strcmp(aTopic, kSmsReceivedObserverTopic)) {
    nsCOMPtr<nsIDOMMozSmsMessage> message = do_QueryInterface(aSubject);
    if (!message) {
      NS_ERROR("Got a 'sms-received' topic without a valid message!");
      return NS_OK;
    }

    unused << SendNotifyReceivedMessage(static_cast<SmsMessage*>(message.get())->GetData());
    return NS_OK;
  }

  if (!strcmp(aTopic, kSmsSendingObserverTopic)) {
    nsCOMPtr<nsIDOMMozSmsMessage> message = do_QueryInterface(aSubject);
    if (!message) {
      NS_ERROR("Got a 'sms-sending' topic without a valid message!");
      return NS_OK;
    }

    unused << SendNotifySendingMessage(static_cast<SmsMessage*>(message.get())->GetData());
    return NS_OK;
  }

  if (!strcmp(aTopic, kSmsSentObserverTopic)) {
    nsCOMPtr<nsIDOMMozSmsMessage> message = do_QueryInterface(aSubject);
    if (!message) {
      NS_ERROR("Got a 'sms-sent' topic without a valid message!");
      return NS_OK;
    }

    unused << SendNotifySentMessage(static_cast<SmsMessage*>(message.get())->GetData());
    return NS_OK;
  }

  if (!strcmp(aTopic, kSmsFailedObserverTopic)) {
    nsCOMPtr<nsIDOMMozSmsMessage> message = do_QueryInterface(aSubject);
    if (!message) {
      NS_ERROR("Got a 'sms-failed' topic without a valid message!");
      return NS_OK;
    }

    unused << SendNotifyFailedMessage(static_cast<SmsMessage*>(message.get())->GetData());
    return NS_OK;
  }

  if (!strcmp(aTopic, kSmsDeliverySuccessObserverTopic)) {
    nsCOMPtr<nsIDOMMozSmsMessage> message = do_QueryInterface(aSubject);
    if (!message) {
      NS_ERROR("Got a 'sms-delivery-success' topic without a valid message!");
      return NS_OK;
    }

    unused << SendNotifyDeliverySuccessMessage(static_cast<SmsMessage*>(message.get())->GetData());
    return NS_OK;
  }

  if (!strcmp(aTopic, kSmsDeliveryErrorObserverTopic)) {
    nsCOMPtr<nsIDOMMozSmsMessage> message = do_QueryInterface(aSubject);
    if (!message) {
      NS_ERROR("Got a 'sms-delivery-error' topic without a valid message!");
      return NS_OK;
    }

    unused << SendNotifyDeliveryErrorMessage(static_cast<SmsMessage*>(message.get())->GetData());
    return NS_OK;
  }

  return NS_OK;
}

bool
SmsParent::RecvHasSupport(bool* aHasSupport)
{
  *aHasSupport = false;

  nsCOMPtr<nsISmsService> smsService = do_GetService(SMS_SERVICE_CONTRACTID);
  NS_ENSURE_TRUE(smsService, true);

  smsService->HasSupport(aHasSupport);
  return true;
}

bool
SmsParent::RecvGetSegmentInfoForText(const nsString& aText,
                                     SmsSegmentInfoData* aResult)
{
  aResult->segments() = 0;
  aResult->charsPerSegment() = 0;
  aResult->charsAvailableInLastSegment() = 0;

  nsCOMPtr<nsISmsService> smsService = do_GetService(SMS_SERVICE_CONTRACTID);
  NS_ENSURE_TRUE(smsService, true);

  nsCOMPtr<nsIDOMMozSmsSegmentInfo> info;
  nsresult rv = smsService->GetSegmentInfoForText(aText, getter_AddRefs(info));
  NS_ENSURE_SUCCESS(rv, true);

  int segments, charsPerSegment, charsAvailableInLastSegment;
  if (NS_FAILED(info->GetSegments(&segments)) ||
      NS_FAILED(info->GetCharsPerSegment(&charsPerSegment)) ||
      NS_FAILED(info->GetCharsAvailableInLastSegment(&charsAvailableInLastSegment))) {
    NS_ERROR("Can't get attribute values from nsIDOMMozSmsSegmentInfo");
    return true;
  }

  aResult->segments() = segments;
  aResult->charsPerSegment() = charsPerSegment;
  aResult->charsAvailableInLastSegment() = charsAvailableInLastSegment;
  return true;
}

bool
SmsParent::RecvPSmsRequestConstructor(PSmsRequestParent* aActor,
                                      const IPCSmsRequest& aRequest)
{
  SmsRequestParent* actor = static_cast<SmsRequestParent*>(aActor);

  switch (aRequest.type()) {
    case IPCSmsRequest::TSendMessageRequest:
      return actor->DoRequest(aRequest.get_SendMessageRequest());
    case IPCSmsRequest::TGetMessageRequest:
      return actor->DoRequest(aRequest.get_GetMessageRequest());
    case IPCSmsRequest::TDeleteMessageRequest:
      return actor->DoRequest(aRequest.get_DeleteMessageRequest());
    case IPCSmsRequest::TMarkMessageReadRequest:
      return actor->DoRequest(aRequest.get_MarkMessageReadRequest());
    default:
      MOZ_NOT_REACHED("Unknown type!");
      break;
  }

  return false;
}

PSmsRequestParent*
SmsParent::AllocPSmsRequest(const IPCSmsRequest& aRequest)
{
  return new SmsRequestParent();
}

bool
SmsParent::DeallocPSmsRequest(PSmsRequestParent* aActor)
{
  delete aActor;
  return true;
}

bool
SmsParent::RecvPMobileMessageCursorConstructor(PMobileMessageCursorParent* aActor,
                                               const IPCMobileMessageCursor& aRequest)
{
  MobileMessageCursorParent* actor =
    static_cast<MobileMessageCursorParent*>(aActor);

  switch (aRequest.type()) {
    case IPCMobileMessageCursor::TCreateMessageCursorRequest:
      return actor->DoRequest(aRequest.get_CreateMessageCursorRequest());
    case IPCMobileMessageCursor::TCreateThreadCursorRequest:
      return actor->DoRequest(aRequest.get_CreateThreadCursorRequest());
    default:
      MOZ_NOT_REACHED("Unknown type!");
      break;
  }

  return false;
}

PMobileMessageCursorParent*
SmsParent::AllocPMobileMessageCursor(const IPCMobileMessageCursor& aRequest)
{
  return new MobileMessageCursorParent();
}

bool
SmsParent::DeallocPMobileMessageCursor(PMobileMessageCursorParent* aActor)
{
  delete aActor;
  return true;
}

/*******************************************************************************
 * SmsRequestParent
 ******************************************************************************/

NS_IMPL_ISUPPORTS1(SmsRequestParent, nsIMobileMessageCallback)

void
SmsRequestParent::ActorDestroy(ActorDestroyReason aWhy)
{
}

bool
SmsRequestParent::DoRequest(const SendMessageRequest& aRequest)
{
  nsCOMPtr<nsISmsService> smsService = do_GetService(SMS_SERVICE_CONTRACTID);
  NS_ENSURE_TRUE(smsService, false);

  return NS_SUCCEEDED(smsService->Send(aRequest.number(), aRequest.message(), this));
}

bool
SmsRequestParent::DoRequest(const GetMessageRequest& aRequest)
{
  nsCOMPtr<nsIMobileMessageDatabaseService> dbService =
    do_GetService(MOBILE_MESSAGE_DATABASE_SERVICE_CONTRACTID);
  NS_ENSURE_TRUE(dbService, false);

  return NS_SUCCEEDED(dbService->GetMessageMoz(aRequest.messageId(), this));
}

bool
SmsRequestParent::DoRequest(const DeleteMessageRequest& aRequest)
{
  nsCOMPtr<nsIMobileMessageDatabaseService> dbService =
    do_GetService(MOBILE_MESSAGE_DATABASE_SERVICE_CONTRACTID);
  NS_ENSURE_TRUE(dbService, false);

  return NS_SUCCEEDED(dbService->DeleteMessage(aRequest.messageId(), this));
}

bool
SmsRequestParent::DoRequest(const MarkMessageReadRequest& aRequest)
{
  nsCOMPtr<nsIMobileMessageDatabaseService> dbService =
    do_GetService(MOBILE_MESSAGE_DATABASE_SERVICE_CONTRACTID);
  NS_ENSURE_TRUE(dbService, false);

  return NS_SUCCEEDED(dbService->MarkMessageRead(aRequest.messageId(),
                                          aRequest.value(), this));
}

nsresult
SmsRequestParent::SendReply(const MessageReply& aReply)
{
  return Send__delete__(this, aReply) ? NS_OK : NS_ERROR_FAILURE;
}

// nsIMobileMessageCallback

NS_IMETHODIMP
SmsRequestParent::NotifyMessageSent(nsISupports *aMessage)
{
  SmsMessage* message = static_cast<SmsMessage*>(aMessage);
  return SendReply(MessageReply(ReplyMessageSend(message->GetData())));
}

NS_IMETHODIMP
SmsRequestParent::NotifySendMessageFailed(int32_t aError)
{
  return SendReply(MessageReply(ReplyMessageSendFail(aError)));
}

NS_IMETHODIMP
SmsRequestParent::NotifyMessageGot(nsISupports *aMessage)
{
  SmsMessage* message = static_cast<SmsMessage*>(aMessage);
  return SendReply(MessageReply(ReplyGetMessage(message->GetData())));
}

NS_IMETHODIMP
SmsRequestParent::NotifyGetMessageFailed(int32_t aError)
{
  return SendReply(MessageReply(ReplyGetMessageFail(aError)));
}

NS_IMETHODIMP
SmsRequestParent::NotifyMessageDeleted(bool aDeleted)
{
  return SendReply(MessageReply(ReplyMessageDelete(aDeleted)));
}

NS_IMETHODIMP
SmsRequestParent::NotifyDeleteMessageFailed(int32_t aError)
{
  return SendReply(MessageReply(ReplyMessageDeleteFail(aError)));
}

NS_IMETHODIMP
SmsRequestParent::NotifyMessageMarkedRead(bool aRead)
{
  return SendReply(MessageReply(ReplyMarkeMessageRead(aRead)));
}

NS_IMETHODIMP
SmsRequestParent::NotifyMarkMessageReadFailed(int32_t aError)
{
  return SendReply(MessageReply(ReplyMarkeMessageReadFail(aError)));
}

/*******************************************************************************
 * MobileMessageCursorParent
 ******************************************************************************/

NS_IMPL_ISUPPORTS1(MobileMessageCursorParent, nsIMobileMessageCursorCallback)

void
MobileMessageCursorParent::ActorDestroy(ActorDestroyReason aWhy)
{
}

bool
MobileMessageCursorParent::RecvContinue()
{
  MOZ_ASSERT(mContinueCallback);

  return NS_SUCCEEDED(mContinueCallback->HandleContinue());
}

bool
MobileMessageCursorParent::DoRequest(const CreateMessageCursorRequest& aRequest)
{
  nsCOMPtr<nsIMobileMessageDatabaseService> dbService =
    do_GetService(MOBILE_MESSAGE_DATABASE_SERVICE_CONTRACTID);
  NS_ENSURE_TRUE(dbService, false);

  nsCOMPtr<nsIDOMMozSmsFilter> filter = new SmsFilter(aRequest.filter());
  bool reverse = aRequest.reverse();

  nsresult rv = dbService->CreateMessageCursor(filter, reverse, this,
                                               getter_AddRefs(mContinueCallback));
  return NS_SUCCEEDED(rv);
}

bool
MobileMessageCursorParent::DoRequest(const CreateThreadCursorRequest& aRequest)
{
  nsCOMPtr<nsIMobileMessageDatabaseService> dbService =
    do_GetService(MOBILE_MESSAGE_DATABASE_SERVICE_CONTRACTID);
  NS_ENSURE_TRUE(dbService, false);

  nsresult rv = dbService->CreateThreadCursor(this,
                                              getter_AddRefs(mContinueCallback));
  return NS_SUCCEEDED(rv);
}

// nsIMobileMessageCursorCallback

NS_IMETHODIMP
MobileMessageCursorParent::NotifyCursorError(int32_t aError)
{
  MOZ_ASSERT(mContinueCallback);

  mContinueCallback = nullptr;

  return SendNotifyError(aError) ? NS_OK : NS_ERROR_FAILURE;
}

NS_IMETHODIMP
MobileMessageCursorParent::NotifyCursorResult(nsISupports* aResult)
{
  MOZ_ASSERT(mContinueCallback);

  nsCOMPtr<nsIDOMMozSmsMessage> iMessage = do_QueryInterface(aResult);
  if (iMessage) {
    SmsMessage* message = static_cast<SmsMessage*>(aResult);
    return SendNotifyResult(MobileMessageCursorData(message->GetData()))
      ? NS_OK : NS_ERROR_FAILURE;
  }

  nsCOMPtr<nsIDOMMozMobileMessageThread> iThread = do_QueryInterface(aResult);
  if (iThread) {
    MobileMessageThread* thread = static_cast<MobileMessageThread*>(aResult);
    return SendNotifyResult(MobileMessageCursorData(thread->GetData()))
      ? NS_OK : NS_ERROR_FAILURE;
  }

  MOZ_NOT_REACHED("Received invalid response parameters!");
  return NS_ERROR_FAILURE;
}

NS_IMETHODIMP
MobileMessageCursorParent::NotifyCursorDone()
{
  MOZ_ASSERT(mContinueCallback);

  mContinueCallback = nullptr;

  return SendNotifyDone() ? NS_OK : NS_ERROR_FAILURE;
}

} // namespace mobilemessage
} // namespace dom
} // namespace mozilla
