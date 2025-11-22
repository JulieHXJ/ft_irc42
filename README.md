# **ft_irc Project Notes**

---

## **Mandatory Requirements**

* Handle **multiple clients** concurrently using `poll()`.
* Use **TCP/IPv4** sockets.
* Implement at least:

  ```
  PASS, NICK, USER, JOIN, PART, PRIVMSG, QUIT
  ```

  and **basic channel management**:

  ```
  MODE, TOPIC, INVITE, KICK (with operator checks)
  ```
* Handle signals properly (`SIGINT`, `SIGQUIT`, `CTRL+D`, `CTRL+Z`).
* Ensure **no memory leaks** (including “still reachable”).

---

## **Bonus Features**

Implemented: **IRC Bot**

---

## **Build & Run**

### **Build**

```bash
make
```

### **Start the Server**

```bash
./ircserv <port> <password>
```

**Example:**

```bash
./ircserv 6667 mypass
```

### **Connect a Client (manual test)**

```bash
nc -C 127.0.0.1 6667
```

> `-C` ensures correct IRC line endings (`\r\n`).

---

## **Core Classes Overview**

### 🖥 **Server**

**Responsibilities:**

* `serverInit()` — create, bind, listen sockets.
* `run()` — main loop:

  * `poll()` events → `accept()`, `recv()`, `send()`.
* Manage:

  * `pollfds` list
  * `client_lst` and `channel_lst`
  * signal pipe
  * server password & uptime
* Clean disconnects and handle shutdown gracefully.

---

### 👤 **Client**

**Represents each connection:**

* `int fd`
* `bool registered`
* `std::string nick, username, realname`
* `std::string host`
* `std::set<std::string> joinedChannels`
* `bool pass_ok`
* Buffers for partial reads/writes (`inbuf`, `outbuf`)

---

### 💬 **Channel**

**Manages state of each IRC channel:**

* `std::string name, topic`
* **Modes supported:**

  * `+i` invite-only
  * `+t` topic only editable by operators
  * `+k` password-protected
  * `+l` user limit
  * `+o` operator management
* Member maps:

  ```cpp
  std::map<std::string, Client*> members;
  std::map<std::string, Client*> operators;
  std::set<std::string> invitedUsers;
  ```
* Helper methods:

  * `isMember()`, `isOperator()`, `isInvited()`
  * `broadcastInChan()`
  * `setMode(flag, state, param)`

---

## **Implemented Commands**

### **Authentication**

```
PASS <password>
NICK <nickname>
USER <nickname> u 0 * :<username>
```

→ When all three complete successfully → client is **registered**.

---

### **Channel / Messaging**

| Command                          | Description                     |
| -------------------------------- | ------------------------------- |
| `JOIN #chan [key]`               | Join or create channel          |
| `PART #chan [:reason]`           | Leave a channel                 |
| `PRIVMSG <target> :<msg>`        | Send message to user or channel |
| `TOPIC #chan [:topic]`           | Get or set topic                |
| `MODE #chan +/-[itkol] [params]` | Manage channel modes            |
| `INVITE <nick> #chan`            | Invite user                     |
| `KICK #chan <nick> [:reason]`    | Remove user                     |
| `QUIT [:reason]`                 | Disconnect                      |

---

### **Supported Channel Modes**

| Mode | Meaning             | Arg            | Example              |
| ---- | ------------------- | -------------- | -------------------- |
| `+i` | Invite-only         | No             | `MODE #tea +i`       |
| `+t` | Topic restricted    | No             | `MODE #tea +t`       |
| `+k` | Channel key         | Yes (`<key>`)  | `MODE #tea +k pass`  |
| `+l` | User limit          | Yes (`<n>`)    | `MODE #tea +l 20`    |
| `+o` | Operator add/remove | Yes (`<nick>`) | `MODE #tea +o alice` |

---

## 🤖 **IRC Bot (Bonus)**

### **Compile**

```bash
make
```

### **Run**

```bash
./ircbot <host> <port> <password> [nickname]
```

**Example:**

```bash
./ircbot 127.0.0.1 6667 mypass ircbot
```

---

### **Bot Behavior**

| Command                        | Description                |
| ------------------------------ | -------------------------- |
| `INVITE ircbot #chan`          | Bot auto joins and says hi |
| `PRIVMSG ircbot :!help`        | List commands              |
| `PRIVMSG ircbot :!ping`        | Responds with `pong`       |
| `PRIVMSG ircbot :!echo <text>` | Echoes `<text>`            |
| `PRIVMSG ircbot :!roll [N]`    | Rolls a dice 1–N           |
| `PRIVMSG ircbot :!time`        | Shows current time         |
| `PRIVMSG ircbot :!quit`        | Bot says bye and exits     |
| `PRIVMSG #chan :!ping`         | Bot responds in channel    |

**Notes:**

* Bot communicates with the server via **its own socket fd**.
* The server is responsible for routing messages between bot and users.
* The bot only reacts to messages it receives; it cannot directly access other clients or channels.
* Gracefully exits when receiving `!quit` or SIGINT.

---

## **Key Implementation Details**

* All sockets are **non-blocking** (`fcntl(fd, F_SETFL, O_NONBLOCK)`).
* Read and write buffers handle **partial messages**.
* All message lines end with `\r\n`.
* Enable `POLLOUT` only when `outbuf` is non-empty.
* Ignore SIGPIPE: `signal(SIGPIPE, SIG_IGN)`.
* `accept()` and `recv()` loops must check `EAGAIN` / `EWOULDBLOCK`.
* Cleanly close sockets on disconnect or error.
* Log every major event (`[INFO]`, `[WARN]`, `[ERR]`, `[DBG]`).

---

## 📚 **References**

* [Beej’s Guide to Network Programming](https://beej.us/guide/bgnet/pdf/bgnet_a4_c_1.pdf)
* [Modern IRC Protocol Docs](https://modern.ircdocs.horse/)
* [chirc – a simple IRC implementation guide](http://chi.cs.uchicago.edu/chirc/irc.html)

---

## **Acknowledgements**

* It was a pleasure building this project together. Huge thanks to my teammates [**mmonika**](https://github.com/mmoniX) and [**gahmed**](https://github.com/ahmed6394) for their dedication, collaboration, and great teamwork throughout the project.
