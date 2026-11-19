# Thread-Pool

allows you to enqueue tasks to be executed by a pool of worker threads.
no need to manage threads yourself, just enqueue tasks and let the pool handle them.

ThreadPool destructor wait for all already-enqueued tasks to finish before shutting down the workers.