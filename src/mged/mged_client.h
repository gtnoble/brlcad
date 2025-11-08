#ifndef MGED_CLIENT_H
#define MGED_CLIENT_H

#include "ged.h"
#include "bu.h"

// Client connection states
enum client_state {
    CLIENT_NEW,
    CLIENT_READING,
    CLIENT_PROCESSING,
    CLIENT_WRITING,
    CLIENT_CLOSING
};

// Client session structure
struct client_session {
    int fd;
    struct ged *gedp;
    struct bu_vls input_buffer;
    struct bu_vls output_buffer;
    enum client_state state;
    int poll_index;  // Index in poll array
};

// Client API
struct client_session *create_client_session(int fd);
void free_client_session(struct client_session *client);
void reset_client_buffers(struct client_session *client);

#endif /* MGED_CLIENT_H */
