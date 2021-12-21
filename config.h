
int PREVIEW_BORDER_COLOR[3] = {40, 74, 53};
int MAIN_BORDER_COLOR[3] = {60, 103, 84};

int SELECTED_BG[3] = {60, 103, 84};
int SELECTED_FG[3] = {0, 0, 0};

int PREVIEW_COMMENT[3] = {100, 100, 100};
int ERROR[3] = {180, 60, 60};


/* Character representing D-Type */
const char DTC[] = {
 [DT_BLK] =     'b', /* block device */
 [DT_CHR] =     'c', /* character device */
 [DT_DIR] =     'd', /* directory */
 [DT_FIFO] =    'F', /* named pipe [FIFO] */
 [DT_LNK] =     'l', /* symbolic link */
 [DT_REG] =     'f', /* regular file */
 [DT_SOCK] =    's', /* UNIX domain socket */
 [DT_UNKNOWN] = '?', /* unknown */
};

/* Size Unit Descriptors */
const char * SUNIT[] = {"B", "KiB", "MiB", "GiB", "TiB"};
