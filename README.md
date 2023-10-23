# P2P File Sharing System - README

## Overview
This project aims to build a peer-to-peer file sharing system with a distributed architecture. Users can share and download files within the groups they belong to. The system allows parallel downloads with multiple pieces from various peers.

## File Division and Hashing
- Files are divided into logical "pieces" of 512KB each.
- SHA1 hashing is used for integrity checks, with the first 20 characters of each hash forming a unique identifier.

## Authentication and Error Handling
- Authentication is required for login.
- Comprehensive error handling has been implemented to ensure the stability of the system.

## Torrent Architecture
- The project implements a custom version of the torrent architecture, deviating from exact BitTorrent protocols to introduce a unique algorithm and architecture.

## Piece Selection Algorithm
- A custom piece selection algorithm has been designed for efficient file sharing among peers.

## Directory Structure
|---tracker—-|
|    |---tracker.cpp
|
|---client—--|
|    |---client.cpp


## Architecture Overview
### Trackers
1. Synchronized trackers (2 tracker system):
   - Maintain client information and shared files data for peer communication.
   - Ensure synchronization between trackers.

### Clients
1. User Authentication:
   - Create an account and register with a tracker.
   - Login using user credentials.

2. Group Management:
   - Create, join, and leave groups.
   - Accept or reject group join requests.

3. File Sharing:
   - Share files within groups, providing filename and SHA1 hash information to the tracker.

4. Download:
   - Fetch peer information from the tracker for file downloads.
   - Implement parallel downloads from multiple peers using a custom piece selection algorithm.

5. Additional Commands:
   - Show downloads, stop sharing files, and logout functionalities.

## Working
1. At least one tracker is always online.
2. Clients create accounts and can own multiple groups.
3. Clients need to join a group to download files.
4. File sharing is done within groups, and filenames and SHA1 hashes are shared with the tracker.
5. Clients send join requests to group owners.
6. After joining, clients can see a list of shareable files in the group.
7. Clients can share files in any group, with only the peer information uploaded to the tracker.
8. Clients request downloads from the tracker, and peer details are provided.
9. Clients communicate with peers to decide which parts of data to download using the custom piece selection algorithm.
10. Downloaded pieces are made available for sharing.
11. After logout, clients temporarily stop sharing files until the next login.
12. All trackers are synchronized with each other.

## Commands
### Tracker
- Run Tracker: `./tracker tracker_info.txt tracker_no`
- Close Tracker: `quit`

### Client
- Run Client: `./client <IP>:<PORT> tracker_info.txt`
- Commands: See the provided list of commands below.
a. Run Client: ./client <IP>:<PORT> tracker_info.txt
tracker_info.txt - Contains ip, port details of all the trackers
b. Create User Account: create_user <user_id> <password>
c. Login: login <user_id> <password>
d. Create Group: create_group <group_id>
e. Join Group: join_group <group_id>
f. Leave Group: leave_group <group_id>
g. List Pending Join: list_requests<group_id>
h. Accept Group Joining Request:
accept_request <group_id> <user_id>
i. List All Group In Network: list_groups
j. List All sharable Files In Group: list_files <group_id>
k. Upload File: upload_file <file_path> <group_id>
l. Download File:
download_file <group_id> <file_name>
<destination_path>
m. Logout: logout
n. Show_downloads: show_downloads
Output format:
[D] [grp_id] filename
[C] [grp_id] filename D(Downloading), C(Complete)
o. Stop sharing: stop_share <group_id> <file_name>
