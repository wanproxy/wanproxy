o) Merge TimeoutThread into CallbackThread?  Some functional reasons to do so,
   namely related to exiting, but is it aesthetic or correct?  Should every
   thread that can run Callbacks also be able to schedule timeouts?
   o) It seems like the right solution to this is pretty unsurprising.  Just
      have each thread have its own predicates that need to be satisfied before
      it will actuall heed a stop request, like the CallbackThread does.  So
      EventPoll waits for no more registered handlers, the I/O thread does
      likewise, the timeout code does likewise, etc.  We'd want to be a little
      more conservative somehow, since we might have one fd being worked on and
      need to service multiple reads or writes before quitting, right?  Also,
      this all moves up the need for stop callbacks to actually get called!
o) Fix the race noted in CallbackThread for inflight callbacks being cancelled
   by another thread.  This could require substantial work, or just passing a
   mutex to callbacks.  If we move to passing a mutex to callbacks, we can go
   the distance of having multiple CallbackThreads immediately.  But how many
   classes would that impact?
o) Fix races with kqueue EventPoll.
o) Proper stop handling for EventThread.
o) Convert other poll mechanisms.
o) Make TimeoutQueue/CallbackQueue suck less.
o) Make signals suck slightly less, maybe.
   XXX Seriously, signal interrupt handling sucks.
o) Since XCodec pipes are consumer-producer now, we can defer production to a
   thread, or at least do the real work in a thread and then have the pipes get
   work to and from the thread.  It should be easy enough to abstract.  It
   might be nice to have a generic framework for doing pipe processing for any
   kind of pipe or maybe just a consumer-producer pipe in a specified thread.
   Then the XCodec could just create a thread for each instance.
