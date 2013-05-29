o) Atomic refcounts for BufferSegment.
o) Proper stop handling for EventThread.
o) Convert other poll mechanisms.
o) Make TimeoutQueue/CallbackQueue suck less.
o) Make signals suck slightly less, maybe.
o) Since XCodec pipes are consumer-producer now, we can defer production to a
   thread, or at least do the real work in a thread and then have the pipes get
   work to and from the thread.  It should be easy enough to abstract.  It
   might be nice to have a generic framework for doing pipe processing for any
   kind of pipe or maybe just a consumer-producer pipe in a specified thread.
   Then the XCodec could just create a thread for each instance.
