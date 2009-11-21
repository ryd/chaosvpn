#define TINC_DEFAULT_PORT "665"

int tinc_generate_config(struct buffer *output, struct config *config);
int tinc_generate_peer_config(struct buffer *output, struct peer_config *peer);
int tinc_generate_up(struct buffer *output, struct config *config);

