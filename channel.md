## 🏗️ Phase 1: Core Structure & Basic Methods

#### 1. Constructor & Destructor
- [x] Implement `Channel(const std::string& name)` constructor
- [x] Implement `~Channel()` destructor (clean up resources)

#### 2. Basic Getters
- [x] `std::string getName() const` - return channel name
- [x] `std::string getTopic() const` - return current topic
- [x] `int getMemberCount() const` - return number of members
- [x] `bool isFull() const` - check if channel reached user limit

#### 3. Member Management
- [x] `bool isMember(const std::string& nickname) const` - check if user is in channel
- [x] `void removeMember(const std::string& nickname)` - remove member from channel

## 🔐 Phase 2: Join & Authentication Logic

#### 4. Join Validation
- [x] `bool canJoin(Client* client, const std::string& password)`
    - [x] Check +i mode and invitation status
    - [x] Check +k mode and password validation
    - [x] Check +l mode and user limit
    - [x] Return appropriate error codes

#### 5. Add Member Logic
- [ ] `bool addMember(Client* client, const std::string& password)`
    - [x] Call `canJoin()` for validation
    - [x] Add to members map
    - [x] Set first joiner as operator (if no operators exist)
    - [x] Send JOIN message to channel
    - [x] Send topic if exists
    - [ ] Send names list

## 👑 Phase 3: Operator & Privilege Management

#### 6. Operator Methods
- [x] `bool isOperator(const std::string& nickname) const` - check operator status
- [x] `void addOperator(const std::string& nickname)` - grant operator privileges
- [x] `void removeOperator(const std::string& nickname)` - remove operator privileges

#### 7. Invite Management
- [x] `void inviteUser(const std::string& nickname)` - add to invited list
- [x] `bool isInvited(const std::string& nickname) const` - check invitation status

## 📢 Phase 4: Message Broadcasting

#### 8. Broadcast Implementation
- [ ] `void broadcast(const std::string &msg, Client* exclude)`
    - [ ] Iterate through all members
    - [ ] Send message to each member except excluded one
    - [ ] Handle potential send failures

## ⚙️ Phase 5: Mode System

#### 9. Mode Management
- [ ] `void setMode(char mode, bool set, const std::string& param)`
    - [ ] Mode i: Set/remove `inviteOnly` flag
    - [ ] Mode t: Set/remove `topicRestriction` flag
    - [ ] Mode k: Set/remove `passKey` and protection flag
    - [ ] Mode o: Call `addOperator()`/`removeOperator()`
    - [ ] Mode l: Set/remove `maxUserLimit`

- [x] `std::string getModesString() const` - return current active modes as string

## 💬 Phase 6: Topic Management

#### 10. Topic Commands
- [ ] `void setTopic(const std::string& newTopic, Client* setter)`
    - [ ] Check topicRestriction and operator status
    - [ ] Update topic
    - [ ] Broadcast TOPIC message to channel

- [ ] `bool canChangeTopic(const std::string& nickname) const`
    - [ ] Return true if no restriction or user is operator

## 🦵 Phase 7: Kick Command

#### 11. Kick Implementation
- [ ] `bool kickMember(Client* requester, const std::string& targetNickname, const std::string& reason)`
    - [ ] Validate requester is operator
    - [ ] Validate target exists in channel
    - [ ] Remove target from channel
    - [ ] Broadcast KICK message
    - [ ] Send KICK message to target

## 🧪 Phase 8: Testing & Edge Cases

#### 12. Error Handling & Edge Cases
- [ ] Handle duplicate nicknames in members/operators
- [ ] Handle removing non-existent members
- [ ] Validate mode parameters (valid limits, passwords, etc.)
- [ ] Handle broadcast to disconnected clients
- [ ] Test all mode combinations

#### 13. Integration Testing
- [ ] Test join with all mode combinations
- [ ] Test operator privilege escalation
- [ ] Test topic restrictions
- [ ] Test kick authorization
- [ ] Test invite-only workflow