#define TINC_DEFAULT_PORT "665"

int tinc_generate_peer_config(struct buffer *output, struct config *peer);
int tinc_generate_config(struct buffer *output, char *interface, char *name, char *ip, struct list_head *config_list);

