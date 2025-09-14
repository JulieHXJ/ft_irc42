**ft\_irc Project Notes**![][image1]

## **Mandatory Requirements**

* Multi-client handling via one loop and poll().

* TCP/IPv4.

* Implement at least: authentication (PASS/NICK/USER), nickname uniqueness, channels, PRIVMSG, JOIN/PART/QUIT, operators and their commands.

* No memory leaks (including “still reachable”).

* Handle signals (SIGINT, SIGQUIT, CTRL+D, CTRL+Z).

---

## **Bonus Ideas**

* Bot (e.g., DEEZNUTS).

* File transfer.

---

## **Core Classes**

1. ### **Server**

* Responsibilities:

  * Open port, bind, listen (`serverInit()`).

  * Main event loop (`run()`): poll → accept/recv/send.

  * Manage all global state (listening socket, poll list, self-pipe, signals).

2. ### **Client**

* Represents a specific client. Each person who connects has:  
  * `int fd`

  * `bool registered` (true after PASS+NICK+USER complete)

  * `std::string nick, user, realname, host`

  * `std::string inbuf, outbuf` (buffers for recv/send)

  * `std::set<std::string> joined` (channels joined)

3. ### **Channel**

* Each channel has: 

  * `std::string name` (e.g., `#general`)

  * `std::string topic`

  * Modes: `i, t, k, l, o` etc.//?

  * `std::string key` (password, \+k)

  * `size_t maxUsers` (user limit, \+l)  
  * …

* Member sets:

  * `std::set<int> members` a list of who is inside.

  * `std::set<int> operators/admin`

---

## **Commands (recommanded)**

1. ### **Authentication / Identity (mandatory)**

* `PASS <password>`

* `NICK <nickname>`

* `USER <username> 0 * :<realname>`

* After PASS+NICK+USER are all valid → mark as registered and send welcome msg.

2. ### **Messaging (mandatory)**

* `PRIVMSG <target> :<text>`

  * target \= nickname or channel.

* `NOTICE <target> :<text>` (recommended).

3. ### **Channels (mandatory)**

* `JOIN #chan [key]`

* `PART #chan [:reason]`

* `QUIT [:reason]`

* `TOPIC #chan [:new topic]` (restricted to ops if \+t)

* `MODE #chan +/-[itkol] [args]`

  * `+i/-i`: invite-only

  * `+t/-t`: topic protected

  * `+k/-k <key>`: channel key

  * `+l/-l <limit>`: user limit

  * `+o/-o <nick>`: give/remove operator

* `INVITE <nick> #chan`

* `KICK #chan <nick> [:reason]`

⚠️ Extra commands like `ADMIN`, `SHUTDOWN`, `SENDFILE`, `GETFILE`, `DEEZNUTS` are moved to **Bonus**.

---

## **Workflow**

![][image2]

1. Get sockets and poll working:  
   1. Call `poll()` to wait for activity.

   2. If listening socket is readable → `accept()` until `EAGAIN`.

   3. For each client fd:

      1. **POLLIN**:

         1. `recv()` until `EAGAIN`.

         2. Append to input buffer.

         3. Extract complete lines (split on `\n`, strip `\r`).

         4. Parse commands → produce responses.

         5. Responses go into outbuf; if not empty, enable **POLLOUT**.

      2. **POLLOUT**:

         1. Send as much as possible from outbuf.

         2. Remove sent bytes.

         3. If empty, disable POLLOUT.

      3. **POLLHUP/POLLERR** or `recv()==0`:

         1. Cleanly remove client: close fd, delete from maps, notify channels.

2. Add signal handling (SIGINT, SIGQUIT).

3. Implement registration (PASS, NICK, USER):  
   1. Before registration is complete: only allow PASS/NICK/USER/QUIT/PING/PONG.

   2. After PASS, NICK, USER are all valid → mark registered → send welcome RPLs.

   3. Reject duplicate nicknames.

4. Add messaging (JOIN, PRIVMSG, PART, QUIT).

5. Add channel management and operator commands.

6. Optional: implement bonus features.

## **Key Details / Pitfalls**

* **Non-blocking**: set `O_NONBLOCK` on all sockets.

* **Partial reads/writes**: must use inbuf/outbuf for each client.

* **Line endings**: IRC uses `\r\n` — strip `\r` after splitting on `\n`.

* **Only enable POLLOUT when needed** (when outbuf non-empty).

* **Ignore SIGPIPE**: `std::signal(SIGPIPE, SIG_IGN);`.

* **Accept loop**: accept until `EAGAIN`.

* **Cleanup**: always remove fd from pollfds, clients map, and channels on disconnect.

* **nc vs irssi**: nc may split lines unpredictably, while irssi always sends complete CRLF-terminated lines. Always buffer until CRLF.

## **Usage**

make

./ircserv \<port\> \<password\>

\# Example

./ircserv 6667 mypass

nc 127.0.0.1 6667

## **Testing**

### **With `nc`**

./ircserv 6667 mypass

\# Terminal 1

nc 127.0.0.1 6667

PASS mypass

NICK Alice

USER alice 0 \* :Alice

JOIN \#chat

PRIVMSG \#chat :Hello everyone\!

\# Terminal 2

nc 127.0.0.1 6667

PASS mypass

NICK Bob

USER bob 0 \* :Bob

JOIN \#chat

\# Should receive Alice's message

### **With IRC client (irssi/WeeChat)**

* Verify correct CRLF formatting and RPL numeric replies.

* Compare with raw log from a real IRC server (`/RAWLOG OPEN debug.log`).

## **References**

* [Beej’s Guide to Network Programming](https://beej.us/guide/bgnet/pdf/bgnet_a4_c_1.pdf?utm_source=chatgpt.com)

* [Modern IRC Docs](https://modern.ircdocs.horse/?utm_source=chatgpt.com)

* [chirc – a simple IRC guide](http://chi.cs.uchicago.edu/chirc/irc.html?utm_source=chatgpt.com)
