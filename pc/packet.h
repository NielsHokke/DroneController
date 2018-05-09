#ifndef _PACKET_H_
#define _PACKET_H_

// for unsigned 8 bit integer
#include <stdint.h>

#ifdef __cplusplus
extern "C"
{
#endif

/**
 * @brief Composition of the packet from pc to be sent via the UART
 */
typedef struct {
    uint8_t mode;                   ///< mode [0-7]
    uint8_t yaw;                    ///< yaw with trim offset
    uint8_t pitch;                  ///< pitch with trim offset
    uint8_t roll;                   ///< roll with trim offset
    uint8_t lift;                   ///< thrust offset for all props
    uint8_t p1;                     ///< position controller gain 
    uint8_t p2;	                    ///< velocity controller gain
    //uint32_t flags;               ///< any flags (reserved for later)
    //extra_t pre_cb;               ///< structure for other.. (reserved for later)  
} packet_config_t;

/*
typedef struct {
	int mode;                   ///< mode [0-7]
	int yaw;                    ///< yaw with trim offset
	int pitch;                  ///< pitch with trim offset
	int roll;                   ///< roll with trim offset
	int lift;                   ///< thrust offset for all props
	int p1;                     ///< position controller gain 
	int p2;	                    ///< velocity controller gain
	//uint32_t flags;               ///< any flags (reserved for later)
	//extra_t pre_cb;               ///< structure for other.. (reserved for later)	
} packet_config_t;
*/
#ifdef __cplusplus
}
#endif

#endif

