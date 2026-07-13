ifdef HALCYON_LEGACY
	SRC += $(USER_PATH)/splitkb/hlc_encoder/hlc_encoder.c
endif

POST_CONFIG_H += $(USER_PATH)/splitkb/hlc_encoder/config.h

ifdef HLC_ENCODER_REV2
	POST_CONFIG_H += $(USER_PATH)/splitkb/hlc_encoder/config_rev2.h
endif
