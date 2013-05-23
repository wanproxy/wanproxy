With the new multithreading infrastructure, cost for some operationg
has increased due to locking overhead.  For example, the callback
speed test (event/example/callback-speed1) has seen its performance
fall quite far.  The solution here is not to fight this trend, as
we could get excessively-clever with locking to do, but to instead
embrace it, I think, and look at callbacks increasingly as a kind
of message-passing rather than a kind of procedural callback.  With
that in mind, it makes an increasing amount of sense to batch
operations.  For instance, something doing packet processing should
pass vectors of packets rather than one packet at a time, and then
process batches together, to avoid trips through synchronizing/serializing
interfaces like callbacks, if at all possible.  At minimum, that
should hopefully be the trend.  Likewise, for things which are going
to schedule a large number of callbacks (such as EventPoll when
processing events), should probably schedule a batch of callbacks
at once rather than acquiring and dropping the callback lock for
each event.  This could increase latency for the next callback which
was about to be executed, but it drastically decreases latency for
the next cohort of callbacks, and for EventPoll itself, and thus
anything which would be contending on EventPoll's lock.
