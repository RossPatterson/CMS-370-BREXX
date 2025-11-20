/* CMS FBLOCK -- EXEC file execution control block */

typedef struct tfblock {
    char filename[8];            /* Filename */
    char filetype[8];            /* Filetype */
    char filemode[2];            /* Filemode */
    unsigned short lenwords;     /* Extension block size in words (Fs) */
    /* Extension starts here */
    ADLEN *descriptors;          /* Address of descriptor list */
    unsigned int descriptor_len; /* Descriptor list length in bytes */
    char prefix[8];              /* Explicit initial prefix */
    char env_name[8];            /* Explicit environment name */
    void *exit_vector;           /* System Exit vector address */
    int exit_user_word;          /* System Exit User Word */
} FBLOCK;
