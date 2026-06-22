#include <ev.h>
#include <stdio.h>

int main() {
    printf("supported backends: 0x%08x\n", ev_supported_backends());
    printf("embeddable backends: 0x%08x\n", ev_embeddable_backends());
    printf("  EVBACKEND_SELECT: supported=%d, embeddable=%d\n",
        !!(ev_supported_backends() & EVBACKEND_SELECT),
        !!(ev_embeddable_backends() & EVBACKEND_SELECT));
    printf("  EVBACKEND_POLL:   supported=%d, embeddable=%d\n",
        !!(ev_supported_backends() & EVBACKEND_POLL),
        !!(ev_embeddable_backends() & EVBACKEND_POLL));
    printf("  EVBACKEND_KQUEUE: supported=%d, embeddable=%d\n",
        !!(ev_supported_backends() & EVBACKEND_KQUEUE),
        !!(ev_embeddable_backends() & EVBACKEND_KQUEUE));
    return 0;
}
