# Community accounts — design only, not implemented

Recorded 16 October 2026. The user explicitly deferred implementation. Current
builds still use automatic DPAPI-protected per-server profiles. No account UI,
password dependency, authentication endpoint or database migration is enabled
by this document.

## Outcome and scope

Let a player recover the same persistent player ID and leaderboard records on
another PC or after reinstalling Windows. Priorities are easy automatic sign-in,
reliable Windows connections and preserving existing profiles/records. Offline
and LAN remain usable without accounts, authentication servers or Internet.

First version: username/password registration and login, Remember me, logout,
password change, recovery codes, existing-profile linking and session revocation.
Email delivery, external identity providers, MFA and account merging are later
options; none is required for the initial small community. Accounts identify
credential holders, not verified real-world people, and do not solve cheating.

## Identity and sessions

- Permanent account ID owns the existing permanent player ID. Leaderboard rows
  continue to use that player ID. IP addresses and hardware IDs are not identity.
- Unique case-insensitive login name (initial proposal: ASCII letters, digits,
  underscore; 3–24 characters). Separate editable Unicode display name; display
  names must not imply verified account ownership in future UI.
- Store salted Argon2id password hashes with versioned parameters, never plaintext
  or reversible passwords. Benchmark the maintained library on the target VDS.
  Login hashing uses a small bounded worker pool/queue; it must not block the
  UDP event loop or exhaust relay memory/CPU under attempted password guessing.
- Use HTTPS, opaque random short-lived access tokens and per-device rotating
  refresh tokens. Store only token digests server-side. Windows DPAPI protects
  remembered refresh tokens; do not store the password on disk. Suggested initial
  policy: 15-minute access lifetime and 90-day idle refresh lifetime, subject to
  testing. Password changes/recovery revoke older sessions as appropriate.
- Automatic refresh happens before control requests and retries once when
  needed. Expiring an access token must not tear down an already authenticated
  running race. New login/control operations still require valid authorization.
  UDP tickets, per-packet MACs and endpoint proof remain separate from passwords.
- Each PC gets a separate remembered session. Provide revoke-other-sessions and
  sign-out controls without coupling legitimate users behind the same router.

## Migration without resetting scores

1. Back up the database; add account/session/recovery tables transactionally.
   Associate at most one account with each existing player ID, with uniqueness
   enforced in SQLite rather than only in request handlers.
2. Create or sign in to an account, then explicitly link the local legacy profile
   by proving its existing secret over HTTPS. Never accept a display name,
   supplied player ID or public score as ownership proof. Bind the authenticated
   account and exact profile atomically; never expose the secret in logs.
3. Keep the player ID and scores. A new PC login obtains the same player ID and
   a fresh per-device session; copying DPAPI files is unnecessary.
4. Reject duplicate claims and silent merging/overwriting. If an account and a
   legacy profile already have different scores/player IDs, require a separately
   specified migration decision before implementation. Preserve legacy support
   during transition, then define whether/when linking rotates legacy access.

An already-lost legacy secret cannot be recovered using only a matching name or
IP. Do not promise retroactive ownership recovery. New profile storage should
use a stable server identifier/canonical origin with explicit legacy-path
migration, preserving existing files rather than silently choosing a new ID.

## Recovery and abuse controls

Generate high-entropy one-use recovery codes during registration; display them
once and store only digests. Recovery sets a new password and revokes affected
sessions atomically; consumed codes cannot be replayed. Regenerating codes
invalidates older ones. Without password, active session or recovery codes,
self-service recovery is unavailable. Do not allow unverified claims to replace
another player's credentials. An operator-only recovery policy is a future
decision, not a public administrator/reset endpoint.

Use generic login/recovery errors, bounded credential-processing work and
graduated retry delays. Do not permanently lock a victim's account merely
because someone else guessed their username, or broadly ban a shared NAT IP.
Keep credentials/tokens out of application, proxy, crash and diagnostic logs.
The VDS operator needs deployment/backup access; the project author does not
need ongoing SSH access for users to register and play.

## Implementation order and acceptance criteria

1. Schema backup/migration/rollback and proof-of-ownership linking.
2. Bounded Argon2id login and device sessions, then Windows Remember me UI.
3. Recovery codes, password change and session revocation.
4. Two-PC migration, shared-NAT concurrency and restart/persistence tests before
   considering accounts required for new Online registrations.

Acceptance: existing scores survive linking; another PC recovers the same ID;
wrong passwords/names/IPs cannot claim it; profile collisions do not overwrite
records; refresh/revocation/recovery replay races are safe; packet loss, expired
tokens and login traffic do not interrupt an existing race; Offline and LAN work
with the account service unavailable. Real Windows + community VDS validation is
required. Implementation should be separately authorized after this paper plan.
