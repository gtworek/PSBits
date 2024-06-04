Just playing with AV, detections, sandboxes and VT. The idea is to make it more sophisticated each round, but let's see how it goes in practice. I plan to randomly change IPs and domain names between rounds.

**NOT A SINGLE BIT OF MY CODE IS INTENTIONALLY MALICIOUS. THE WORST THING YOU CAN EXPECT IS SOME WASTE OF BANDWIDTH/MEMORY/CPU.**

The goal is to:
  - observe running and downloading my code across the world,
  - observe AV signatures changes/spreading,
  - observe bots downloading stuff based on VT data,
  - learn how AVs learn about potentially bad stuff.

Versions (so far, I will gradually add more):
  - v1: a simple process, downloading itself (same hash), launching downloaded content and terminating. Keeping number of processes low, due to parent process termination - https://www.virustotal.com/gui/file/0eb77a3b31656744ed3511085a1aa1369790b24577fdce2f2501786806a9c182
  - v2: same as v1, but a bit less waiting between cycles and DO NOT terminating unless child process exists. Leading effectively to large number of processes - https://www.virustotal.com/gui/file/4924802fc291094c36e123f0d072c0207157a60ac85bd3275766d00691958ced
  - v3: same as v2, but each downloaded copy is binary different (last 4 bytes change randomly each time), leading to unique file hashes. I am providing the source code, but the 'live' service replaces first 8 bytes of each file with `FAKEFAKE`
  - v4: exe is compiled dynamically (TCC), with unique strings (for unique file hashes) and different parts of code, such as imports, included/excluded randomly to manipulate imphash etc.
  - v5: coming soon.


Links to VT will be added with some delay to limit interference between automated and human-driven actions.

Hello mrmpzjjhn3sgtq5w, is it me you're looking for? ;)
