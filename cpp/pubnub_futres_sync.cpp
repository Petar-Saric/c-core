/* -*- c-file-style:"stroustrup"; indent-tabs-mode: nil -*- */
#include "pubnub.hpp"
#include "pubnub_mutex.hpp"

#if PUBNUB_USE_EXTERN_C
extern "C" {
#endif
#include "core/pubnub_ntf_sync.h"
#include "core/pubnub_coreapi.h"
#if PUBNUB_USE_EXTERN_C
}
#endif


namespace pubnub {

class pthread_lock_guard {
public:
    pthread_lock_guard(pubnub_mutex_t mutex) : d_mutex(mutex) {
        pubnub_mutex_lock(d_mutex);
    }
    ~pthread_lock_guard() { pubnub_mutex_unlock(d_mutex); }
private:
    pubnub_mutex_t d_mutex;
};

class futres::impl {

public:
    impl(pubnub_t *pb, pubnub_res initial) :
        d_pb(pb),
        d_result(initial) {
        pubnub_mutex_init(d_mutex);
    }
    impl(impl* pimpl) {
        pthread_lock_guard lck(pimpl->d_mutex);
        impl(pimpl->d_pb, pimpl->d_result);
    }
    ~impl() {
        pubnub_mutex_destroy(d_mutex);
    }
    pubnub_res end_await() {
        pthread_lock_guard lck(d_mutex);
        if (PNR_STARTED == d_result) {
            return d_result = pubnub_await(d_pb);
        }
        else {
            return d_result;
        }
    }
    pubnub_res last_result() {
        pthread_lock_guard lck(d_mutex);
        if (PNR_STARTED == d_result) {
            return d_result = pubnub_last_result(d_pb);
        }
        else {
            return d_result;
        }
    }
    bool is_ready() const {
        pthread_lock_guard lck(d_mutex);
        return d_result != PNR_STARTED;
    }

private:
    pubnub_mutex_t d_mutex;
    /// The C Pubnub context that we are "wrapping"
    pubnub_t* d_pb;
    pubnub_res d_result;
};


futres::futres(pubnub_t *pb, context &ctx, pubnub_res initial) : 
    d_ctx(ctx), d_pimpl(new impl(pb, initial)) 
{
}


#if __cplusplus < 201103L
futres::futres(futres const& x)
    : d_ctx(x.d_ctx)
    , d_pimpl(new impl(x.d_pimpl))
{
}
#endif


futres::~futres() 
{
    delete d_pimpl;
}


pubnub_res futres::last_result()
{
    return d_pimpl->last_result();
}

void futres::start_await()
{
    // nothing to do to "start" a sync await...
}

 
pubnub_res futres::end_await()
{
    return d_pimpl->end_await();
}


bool futres::valid() const
{
    return d_pimpl != 0;
}


bool futres::is_ready() const
{
    return d_pimpl->is_ready();
}

pubnub_publish_res futres::parse_last_publish_result()
{
    return d_ctx.parse_last_publish_result();
}
 
#if (__cplusplus >= 201103L) || (_MSC_VER >= 1600)
void futres::then(std::function<void(context &, pubnub_res)> f)
{
    f(d_ctx, await());
}
#else
void futres::thenx(caller_keeper f)
{
    f(d_ctx, await());
}
#endif

}
