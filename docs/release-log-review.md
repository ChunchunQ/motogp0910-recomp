# Release log review and noise reduction

The supplied Windows log covers approximately 17 minutes, ending in the
ordinary window-close and hard-exit sequence. The user reports Time Trial at
Catalunya followed by a three-lap championship race at Brno. This log contains
no recorded fatal exception, device-loss error or audio failure; it cannot
establish release readiness for other tracks, modes or hardware.

There are 2,240 errors from the same unimplemented `NetDll_XNetGetOpt` option 6,
and 10 `XamUserCreateStatsEnumerator` stub warnings. The configuration selects
LAN mode, where community Online statistics are not available. These remain
unsupported paths, not successful API implementations. The earlier native
analysis in [LAN lobby stability](lan-lobby-stability.md) also identifies a
caller that reads option 6's output without checking the error result.

Routine application traces now use debug level: localization setup, selected
display/aspect, title-menu tracing, collision-record endian repairs, livery
readback, successful adaptive-trigger reports, detailed network startup,
packet/session traces and successful Online statistics writes. Normal `info`
logging retains the runtime's hardware, device and lifecycle context, one
network-ready record identifying the selected mode and adapter, plus warnings
and errors. The default level remains `info`, rather than suppressing useful
diagnostic context with a blanket warning/error-only policy.

Advanced → Enable debug overlay now also selects Debug logging on the next
game launch. Turning the option off returns to normal Info logging. This
policy does not overwrite an explicit log-level, verbose or category override;
`--log_level=debug` remains available for manual diagnosis. The checkbox does
not enable Trace or noisy per-iteration logging, and opening/closing F3 does
not switch logging in the middle of a session.

The application throttles only the known option 6 error, retaining its error
severity and emitting it on occurrence 1, 2, 4, 8 and subsequent powers of two.
The occurrence number exposes continued repetition. The original WSAEINVAL
return and untouched outputs are preserved; other options go to the runtime.
The statistics stub warnings and unrelated failures are not filtered.

The resulting package is compiled without running tests, as requested. The
user's supplied log predates the pending UI and logging changes and does not
validate that new executable.
