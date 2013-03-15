/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "mozilla/dom/ContentChild.h"
#include "SmsIPCService.h"
#include "nsXULAppAPI.h"
#include "jsapi.h"
#include "mozilla/dom/mobilemessage/SmsChild.h"
#include "SmsMessage.h"
#include "SmsFilter.h"
#include "SmsSegmentInfo.h"

namespace mozilla {
namespace dom {
namespace mobilemessage {

PSmsChild* gSmsChild;

NS_IMPL_ISUPPORTS2(SmsIPCService, nsISmsService, nsIMobileMessageDatabaseService)

void
SendRequest(const IPCSmsRequest& aRequest, nsIMobileMessageCallback* aRequestReply)
{
  MOZ_ASSERT(NS_IsMainThread());

  NS_WARN_IF_FALSE(gSmsChild,
                   "Calling methods on SmsIPCService during "
                   "shutdown!");

  if (gSmsChild) {
    SmsRequestChild* actor = new SmsRequestChild(aRequestReply);
    gSmsChild->SendPSmsRequestConstructor(actor, aRequest);
  }
}

PSmsChild*
SmsIPCService::GetSmsChild()
{
  if (!gSmsChild) {
    gSmsChild = ContentChild::GetSingleton()->SendPSmsConstructor();
  }

  return gSmsChild;
}

/*
 * Implementation of nsISmsService.
 */
NS_IMETHODIMP
SmsIPCService::HasSupport(bool* aHasSupport)
{
  GetSmsChild()->SendHasSupport(aHasSupport);

  return NS_OK;
}

NS_IMETHODIMP
SmsIPCService::GetSegmentInfoForText(const nsAString & aText,
                                     nsIDOMMozSmsSegmentInfo** aResult)
{
  SmsSegmentInfoData data;
  bool ok = GetSmsChild()->SendGetSegmentInfoForText(nsString(aText), &data);
  NS_ENSURE_TRUE(ok, NS_ERROR_FAILURE);

  nsCOMPtr<nsIDOMMozSmsSegmentInfo> info = new SmsSegmentInfo(data);
  info.forget(aResult);
  return NS_OK;
}

NS_IMETHODIMP
SmsIPCService::Send(const nsAString& aNumber,
                    const nsAString& aMessage,
                    nsIMobileMessageCallback* aRequest)
{
  SendRequest(SendMessageRequest(nsString(aNumber), nsString(aMessage)), aRequest);
  return NS_OK;
}

/*
 * Implementation of nsIMobileMessageDatabaseService.
 */
NS_IMETHODIMP
SmsIPCService::GetMessageMoz(int32_t aMessageId,
                             nsIMobileMessageCallback* aRequest)
{
  SendRequest(GetMessageRequest(aMessageId), aRequest);
  return NS_OK;
}

NS_IMETHODIMP
SmsIPCService::DeleteMessage(int32_t aMessageId,
                             nsIMobileMessageCallback* aRequest)
{
  SendRequest(DeleteMessageRequest(aMessageId), aRequest);
  return NS_OK;
}

NS_IMETHODIMP
SmsIPCService::CreateMessageCursor(nsIDOMMozSmsFilter* aFilter,
                                   bool aReverse,
                                   nsIMobileMessageCursorCallback* aCallback,
                                   nsICursorContinueCallback** aResult)
{
  NS_ENSURE_TRUE(gSmsChild, NS_ERROR_FAILURE);

  nsRefPtr<MobileMessageCursorChild> actor =
    new MobileMessageCursorChild(aCallback);
  SmsFilterData data =
    SmsFilterData(static_cast<SmsFilter*>(aFilter)->GetData());

  PMobileMessageCursorChild* child =
    gSmsChild->SendPMobileMessageCursorConstructor(actor,
      IPCMobileMessageCursor(CreateMessageCursorRequest(data, aReverse)));
  NS_ENSURE_TRUE(child, NS_ERROR_FAILURE);

  actor.forget(aResult);
  return NS_OK;
}

NS_IMETHODIMP
SmsIPCService::MarkMessageRead(int32_t aMessageId,
                               bool aValue,
                               nsIMobileMessageCallback* aRequest)
{
  SendRequest(MarkMessageReadRequest(aMessageId, aValue), aRequest);
  return NS_OK;
}

NS_IMETHODIMP
SmsIPCService::CreateThreadCursor(nsIMobileMessageCursorCallback* aCallback,
                                  nsICursorContinueCallback** aResult)
{
  NS_ENSURE_TRUE(gSmsChild, NS_ERROR_FAILURE);

  nsRefPtr<MobileMessageCursorChild> actor =
    new MobileMessageCursorChild(aCallback);

  PMobileMessageCursorChild* child =
    gSmsChild->SendPMobileMessageCursorConstructor(actor,
      IPCMobileMessageCursor(CreateThreadCursorRequest()));
  NS_ENSURE_TRUE(child, NS_ERROR_FAILURE);

  actor.forget(aResult);
  return NS_OK;
}

} // namespace mobilemessage
} // namespace dom
} // namespace mozilla
