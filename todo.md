
# ✅ IRC Server Project – To-Do List

## 0. Setup & Basics

* [x] **Makefile** (compile all sources with `-Wall -Wextra -Werror -std=c++98`)
* [x] **Server skeleton** (`Server.hpp` + `Server.cpp`)
* [x] **Main.cpp** with signal handling (`SIGINT`, `SIGQUIT`, ignore `SIGPIPE`)
* [x] **Create listening socket** (`socket`, `setsockopt`, `bind`, `listen`)
* [x] **Non-blocking mode** for sockets (`fcntl O_NONBLOCK`)
* [x] **Poll loop** (`pollfds` vector, accept new clients, recv/send, cleanup)

---

## 1. Client Handling

* [x] Accept multiple clients (`accept` until `EAGAIN`)
* [x] Store per-client buffers (`inbuff`, `outbuff`)
* [ ] ✅ Introduce `Client` struct (fd, ip, port, buffers, nick/user)
* [ ] Switch from `map<int, string>` → `map<int, Client>` (step by step)
* [ ] Add helper: `cleanupClient(fd)` to close & erase

---

## 2. Basic Commands (before registration)

* [ ] `QUIT` → close connection
* [ ] `PING` / `PONG` → keepalive
* [ ] Only allow `PASS`, `NICK`, `USER`, `QUIT`, `PING/PONG` before login

---

## 3. Registration

* [ ] `PASS <password>` → check server password
* [ ] `NICK <nickname>` → store nickname, reject duplicates
* [ ] `USER <username> <hostname> <servername> :<realname>` → store user info
* [ ] When PASS+NICK+USER are valid → mark client registered
* [ ] Send `001–004` welcome replies

---

## 4. Messaging

* [ ] `PRIVMSG <nick> :<message>` → private message to user
* [ ] `PRIVMSG #channel :<message>` → broadcast to channel members
* [ ] `NOTICE` → like PRIVMSG but without auto-replies

---

## 5. Channels

* [ ] Create `Channel` class (name, topic, members, ops, modes)
* [ ] `JOIN #room` → add client to channel
* [ ] `PART #room` → leave channel
* [ ] `QUIT` → leave all channels & close
* [ ] Forward all messages in channel to all members

---

## 6. Operator & Channel Commands

* [ ] `KICK <nick> #channel [:reason]`
* [ ] `INVITE <nick> #channel`
* [ ] `TOPIC #channel [:newtopic]`
* [ ] `MODE #channel <modes>` (at least basic ones: +o, +t, +k, +l)
* [ ] `LIST` → list all channels

---

## 7. Server Management

* [ ] Add server password requirement
* [ ] Add operator password (`OPER`)
* [ ] Add `KILL` (operator command to disconnect user)
* [ ] Properly format all replies (RPLs / ERRs per RFC)

---

## 8. Cleanup & Signals

* [ ] Handle `CTRL+D` (EOF / partial input) correctly
* [ ] Handle `CTRL+Z` (suspend/resume) without corruption
* [ ] Graceful shutdown (close all clients, announce server quitting)
* [ ] Ensure **no memory leaks** (Valgrind clean)

---

## 9. Bonus (optional)

* [ ] Bot command (e.g. `DEEZNUTS` → send joke/info)
* [ ] File transfer (`SENDFILE`, `GETFILE`)
* [ ] Nickname service bot (auto-reply)

---

✅ **Workflow tip**:
Do one block at a time → test with `nc` (netcat) → then test with a real IRC client (irssi, weechat).

