/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

/* Typedefs and structures that represent the libvlc API / ABI */

#ifndef _WRAPPER_TYPES_HXX
#define _WRAPPER_TYPES_HXX

#if defined WNT
        typedef __int64 libvlc_time_t;
#else
        typedef int64_t libvlc_time_t;
#endif

extern "C" {

// basic callback / event types we use
typedef int libvlc_event_type_t;
typedef struct libvlc_event_manager_t libvlc_event_manager_t;
typedef void ( *libvlc_callback_t ) ( const struct libvlc_event_t *, void * );

// the enumeration values we use cf. libvlc_events.h
#define libvlc_MediaPlayerPaused     0x105
#define libvlc_MediaPlayerEndReached 0x109

// event structure pieces we use
struct libvlc_event_t
{
    int   type;  // event type
    void *p_obj; // object emitting that event

    union // so far we don't need this.
    {
      struct {
        const char *dummy1;
        const char *dummy2;
      } padding;
    } u;
};

struct libvlc_track_description_t
{
    int i_id;
    char *psz_name;
    libvlc_track_description_t *p_next;
};

}

#endif // _WRAPPER_TYPES_HXX

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
