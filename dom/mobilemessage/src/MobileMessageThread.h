/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef mozilla_dom_mobilemessage_MobileMessageThread_h
#define mozilla_dom_mobilemessage_MobileMessageThread_h

#include "mozilla/Attributes.h"
#include "mozilla/dom/mobilemessage/SmsTypes.h"
#include "nsIDOMMozMobileMessageThread.h"
#include "nsString.h"
#include "jspubtd.h"

namespace mozilla {
namespace dom {

class MobileMessageThread MOZ_FINAL : public nsIDOMMozMobileMessageThread
{
  typedef mobilemessage::ThreadData ThreadData;

public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIDOMMOZMOBILEMESSAGETHREAD

  static nsresult Create(const uint64_t aId,
                         const nsAString& aBody,
                         const uint64_t aUnreadCount,
                         const JS::Value& aParticipants,
                         const JS::Value& aTimestamp,
                         JSContext* aCx,
                         nsIDOMMozMobileMessageThread** aThread);

  MobileMessageThread(const uint64_t aId,
                      const nsString& aBody,
                      const uint64_t aUnreadCount,
                      const nsTArray<nsString>& aParticipants,
                      const uint64_t aTimestamp);

  MobileMessageThread(const ThreadData& aData);

  const ThreadData& GetData() const { return mData; }

private:
  ThreadData mData;
};

} // namespace dom
} // namespace mozilla

#endif // mozilla_dom_mobilemessage_MobileMessageThread_h
