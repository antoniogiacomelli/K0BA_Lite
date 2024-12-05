#ifndef KMEM_H
#define KMEM_H

#ifdef __cplusplus
extern "C" {
#endif

K_ERR kMemInit(K_MEM* const, ADDR const, BYTE, BYTE const);
ADDR kMemAlloc(K_MEM* const);
K_ERR kMemFree(K_MEM* const, ADDR const);

#ifdef __cplusplus
}
#endif

#endif
