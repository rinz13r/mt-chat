#pragma once

enum Request {
    JOIN_ROOM,
    LEAVE_ROOM,
    SEND_MSG,
};

enum ServerOps {
    MSG,
    BYE,
    NOTIF,
};

typedef long long ll;

struct t_format {
    ll s;
    ll us;
};

struct User {
    char name[20];
    int room;
};

struct Notification {
    char msg[256];
    struct t_format ts;
};
struct Msg {
    int grp;
    struct t_format ts;
    char who[20];
    char msg[256];
};
