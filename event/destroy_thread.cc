/*
 * Copyright (c) 2015 Juli Mallett. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <sched.h>

#include <event/destroy_thread.h>

DestroyThread::DestroyThread(void)
: Thread("DestroyThread"),
  log_("/destroy/thread"),
  mtx_("DestroyThread"),
  sleepq_("DestroyThread", &mtx_),
  idle_(false),
  queue_()
{ }

void
DestroyThread::append(DestroyBase *db)
{
	mtx_.lock();
	bool need_wakeup = queue_.empty();
	queue_.push_back(db);
	if (need_wakeup && idle_)
		sleepq_.signal();
	mtx_.unlock();
}

void
DestroyThread::main(void)
{
	mtx_.lock();
	for (;;) {
		if (queue_.empty()) {
			idle_ = true;
			for (;;) {
				if (stop_) {
					mtx_.unlock();
					return;
				}
				sleepq_.wait();
				if (queue_.empty())
					continue;
				idle_ = false;
				break;
			}
		}

		while (!queue_.empty()) {
			DestroyBase *db = queue_.front();
			queue_.pop_front();

			DEBUG(log_) << "Destroy object " << db->obj() << " with lock " << db->lock_->name() << ".";

			mtx_.unlock();
			db->lock_->lock();
			delete db;
			mtx_.lock();
		}
	}
}
