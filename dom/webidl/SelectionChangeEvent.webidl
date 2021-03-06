/* -*- Mode: IDL; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/.
 */

enum SelectionChangeReason {
  "drag",
  "mousedown",
  "mouseup",
  "keypress",
  "selectall",
  "collapsetostart",
  "collapsetoend"
};

dictionary SelectionChangeEventInit : EventInit {
  DOMString selectedText = "";
  DOMRectReadOnly? boundingClientRect = null;
  sequence<SelectionChangeReason> reasons = [];
};

[Constructor(DOMString type, optional SelectionChangeEventInit eventInit),
 ChromeOnly]
interface SelectionChangeEvent : Event {
  readonly attribute DOMString selectedText;
  readonly attribute DOMRectReadOnly? boundingClientRect;
  [Cached, Pure] readonly attribute sequence<SelectionChangeReason> reasons;
};
