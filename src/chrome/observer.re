open Chrome;

type notification = {.
    "readDate": option(string)
};

type user = {.
    "name": string
};

type storage = {.
    "unread": int,
    "user": string
};

[@bs.val]
external parseNotifications : string => Js.Array.t(notification) = "JSON.parse";

[@bs.val]
external parseUser : string => user = "JSON.parse";

let updateNotifications = () => Js.Promise.(
    all2((Request.request("/notifications"), Request.request("/whoami")))
    |> then_(((notifications, whoami)) =>
        parseNotifications(notifications)
        |> Js.Array.filter((notification) =>
            switch (notification##readDate) {
            | Some(_date) => false
            | None => true
            })
        |> (notifications) => {
            "unread": Array.length(notifications),
            "user": parseUser(whoami)##name
        }
        |> (content) => chrome##storage##sync##set(content)
        |> resolve
    )
    |> catch((_error) => chrome##browserAction##setBadgeText({"text": "..."}) |> resolve))
    |> ignore;

[@bs.new]
external notify : (string, {."icon": string, "body": string}) => unit = "Notification";

let observeNotifications = (changes) => {
    let newValue = changes##unread##newValue;
    let oldValue = changes##unread##oldValue;
    let user = changes##user##newValue;

    switch newValue {
    | 0 => ()
    | n => {
        let text = n > 99 ? "99+" : string_of_int(n);
        chrome##browserAction##setBadgeText({"text": text});
        if (newValue != oldValue) {
            chrome##i18n##getMessage("unreadNotifications")
            |> Js.String.replace("{{AMOUNT}}", text)
            |> (body) => notify(user, {"icon": "/public/resources/rung.png", "body": body})
        }
    }}
};

chrome##browserAction##setBadgeBackgroundColor({"color": "#FFA000"});
chrome##alarms##create({"periodInMinutes": 30});
chrome##storage##sync##set({"unread": 0, "user": ""});

chrome##alarms##onAlarm##addListener(updateNotifications);
chrome##storage##onChanged##addListener(observeNotifications);

updateNotifications();
