# PES-VCS Lab Report
**Name:** N Bhuvan Teja
**SRN:** PES1UG24CS286 

---

## Phase 1 — Object Store
### Screenshot 1A — test_objects passing
[1A](screenshots/1A.png)

### Screenshot 1B — sharded object store
[1B](screenshots/1B.png)

---

## Phase 2 — Tree Objects
### Screenshot 2A — test_tree passing
[2A](screenshots/2A.png)

### Screenshot 2B — xxd raw tree object
[2B](screenshots/2B.png)

---

## Phase 3 — Staging Area
### Screenshot 3A — pes add and pes status
[3A](screenshots/3A.png)

### Screenshot 3B — cat .pes/index
[3B](screenshots/3B.png)

---

## Phase 4 — Commits and History
### Screenshot 4A — pes log with three commits
[4A](screenshots/4A.png)

### Screenshot 4B — find .pes -type f after three commits
[4B](screenshots/4B.png)

### Screenshot 4C — cat .pes/HEAD and cat .pes/refs/heads/main
[4C](screenshots/4C.png)

### Final — Integration test passing
[Final1](screenshots/Final(1).png)
[Final2](screenshots/Final(2).png)

---

## Phase 5 — Analysis: Branching and Checkout

### Q5.1 — How would you implement pes checkout?
A branch in `.pes/refs/heads/` is just a file containing a commit hash.
To switch branches, three things must happen:
1. Read the target branch's commit hash from `.pes/refs/heads/<branch>`
2. Update `.pes/HEAD` to say `ref: refs/heads/<branch>`
3. Walk the target commit's tree and restore every file into the working
   directory by reading each blob from the object store and writing it
   to the correct path on disk. Files present in the old branch but
   absent in the target tree must be deleted to avoid stale files.

The complexity comes from step 3 — you must recursively walk subtrees,
read every blob, and write each file to disk. You also need to handle
deletions of files that don't exist in the target tree.

### Q5.2 — How do you detect a dirty working directory conflict?
For each file in the index, compare the stored mtime and size against
the actual file on disk using stat(). If they differ, the file has been
modified since it was last staged. If the target branch has a different
blob hash for that same file, checking out would silently overwrite the
user's unsaved work — so refuse the checkout and print an error.
No re-hashing is needed; the mtime+size fast-path is sufficient.

### Q5.3 — What happens in detached HEAD state?
Detached HEAD means `.pes/HEAD` contains a raw commit hash instead of
`ref: refs/heads/main`. New commits update HEAD directly but no branch
file is updated. Once you switch branches, those commits become
unreachable. To recover: note the commit hash before switching, then
run `pes branch <new-branch>` pointing to that hash, and switch to it.

---

## Phase 6 — Analysis: Garbage Collection

### Q6.1 — Algorithm to find and delete unreachable objects
Use mark-and-sweep:

**Mark phase:** Start from all branch files under `.pes/refs/heads/`.
For each commit, follow parent pointers to the root. For every commit
visited, add its hash to a reachable set, read its tree, add the tree
hash, recursively walk all subtrees adding each tree and blob hash.
Use a hash table for O(1) membership checks.

**Sweep phase:** Enumerate every file under `.pes/objects/`. For each
one, if its hash is not in the reachable set, delete it.

For 100,000 commits and 50 branches, you would visit roughly
100,000 commits + 100,000 trees + several million blobs. In practice,
shared objects reduce this significantly — expect 500,000–2,000,000
total objects to visit.

### Q6.2 — Race condition between GC and a concurrent commit
1. A commit writes a new blob to the object store. It exists on disk
   but is not yet referenced by any commit or branch.
2. GC runs at this exact moment, does not find the blob in any
   reachable chain, and deletes it.
3. The commit continues, creates a tree and commit object referencing
   the deleted blob, and writes the commit hash to the branch file.
   The repo now has a commit pointing to a non-existent object —
   data corruption.

Real Git avoids this with a grace period: GC never deletes objects
created in the last 2 weeks. Any in-progress operation completes well
within that window. Git also ignores `tmp_*` named files during GC,
protecting objects mid-write.
