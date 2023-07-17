#include "XOPStandardHeaders.r"

resource 'vers' (1) {						/* XOP version info */
	0x01, 0x00, final, 0x00, 0,				/* version bytes and country integer */
	"1.00",
	"1.00, Copyright 1993-2018 WaveMetrics, Inc., all rights reserved."
};

resource 'vers' (2) {						/* Igor version info */
	0x08, 0x00, release, 0x00, 0,			/* version bytes and country integer */
	"8.00",
	"(for Igor Pro 8.00 or later)"
};

resource 'STR#' (1100) {					/* custom error messages */
	{
		/* [1] */
		"ATTPCUtil requires Igor Pro 8.00 or later.",
		/* [2] */
		"ATTPCUtil XOP was called to execute an unknown function.",
		/* [3] */
		"Input string is non-existent.",
	}
};

/* no menu item */

resource 'XOPI' (1100) {
	XOP_VERSION,							// XOP protocol version.
	DEV_SYS_CODE,							// Code for development system used to make XOP
	XOP_FEATURE_FLAGS,						// Tells Igor about XOP features
	XOPI_RESERVED,							// Reserved - must be zero.
	XOP_TOOLKIT_VERSION,					// XOP Toolkit version.
};

resource 'XOPF' (1100) {
	{
        /* dmin = closestRange(cloud,vec,weight) */    /* This uses the direct call method */
        "closestRange",                            /* function name */
        F_UTIL | F_EXTERNAL,                    /* function category */
        NT_FP64,                        /* return value type (double precision float) */
        {
            WAVE_TYPE,                    /* cloud (wave handle) */
            WAVE_TYPE,                    /* vec (wave handle) */
            WAVE_TYPE,                    /* weight (wave handle) */
        },
        
        /* ret = lutDistances(cloud) */    /* This uses the direct call method */
        "lutDistances",                            /* function name */
        F_UTIL | F_EXTERNAL,                    /* function category */
        NT_FP64,                        /* return value type (double precision float) */
        {
            WAVE_TYPE,                    /* cloud (wave handle) */
            WAVE_TYPE,                    /* plook (wave handle) */
        },

        "neighborSmooth",                            /* function name */
        F_UTIL | F_EXTERNAL,                    /* function category */
        NT_FP64,                        /* return value type (double precision float) */
        {
            WAVE_TYPE,                    /* cloud (wave handle) */
            WAVE_TYPE,                    /* plook (wave handle) */
            NT_FP64,                    /* maxdistance (double) */
            NT_FP64,                    /* threshold (double) */
            WAVE_TYPE,                    /* smoothed (wave handle) */
       },

        "calculateDmatrix",                            /* function name */
        F_UTIL | F_EXTERNAL,                    /* function category */
        NT_FP64,                        /* return value type (double precision float) */
        {
            NT_FP64,                    /* scale (double) */
            WAVE_TYPE,                    /* triplets (wave handle) */
            WAVE_TYPE,                    /* matrix (wave handle) */
       },

	}
};
