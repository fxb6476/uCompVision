/*
 *  V4L2 video capture example
 *
 *  This program can be used and distributed without restrictions.
 *
 *      This program is provided with the V4L2 API
 * see https://linuxtv.org/docs.php for more information
 */

#include <opencv2/opencv.hpp>
#include <time.h>
#include <fstream>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include <getopt.h>             /* getopt_long() */

#include <fcntl.h>              /* low-level i/o */
#include <unistd.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/mman.h>
#include <sys/ioctl.h>

#include <linux/videodev2.h>
#include "utils.h"
#include <jpeglib.h>

using namespace std;
using namespace cv;

static char             *dev_name;
static enum io_method   io = IO_METHOD_MMAP;
static int              fd = -1;
struct buffer           *buffers;
static unsigned int     n_buffers;
static int              out_buf;
static int              force_format;
static int              frame_count = 70;
static int 		        pic_height = 1080;
static int 		        pic_width = 1920;

static void process_image(void *img, int size)
{
    if (out_buf)
            //fwrite(img, size, 1, stdout);
            printf("Image size -> %d\n", size);

    fflush(stderr);
    fprintf(stderr, ".");
    fflush(stdout);

    //ofstream outfile ("/stream/pic.jpg", ofstream::binary);
    //outfile.write((char *)img, size);
    //outfile.close();

    // Trying to use libjpeg to decompress mjpeg. See whats better, libjpeg or opencv -.-
    struct jpeg_decompress_struct cinfo;
    struct jpeg_error_mgr jerr;
    int rc;

    // Some error handling?
    cinfo.err = jpeg_std_error(&jerr);

    // Creating object and setting memory in structure...
    jpeg_create_decompress(&cinfo);
    jpeg_mem_src(&cinfo, (unsigned char *)img, size);

    // Checking if it is a readl jpeg file... Should work?
    rc = jpeg_read_header(&cinfo, TRUE);

    if (rc != 1) {
        printf("File does not seem to be a normal JPEG\n");
    }

    // Starting the decompression...
    jpeg_start_decompress(&cinfo);

    // Now we are only going to decompress the desired width and height.
    // Aka cropping during decompressing?
    // Janky croping of raw RGB data ;) frek opencv  hehe...
    int pixel_size = cinfo.output_components;
    int width = 640;
    int height = 480;
    int cent_x = pic_width / 2;
    int cent_y = pic_height / 2;
    int min_width = cent_x - (width / 2);
    int min_height = cent_y - (height / 2);
    int max_height = cent_y + (height / 2);

    // Setting cropping dimensions before we start reading the decompressed data...
    printf("Cropped image is %d by %d with %d components\n", width, height, 3);
    JDIMENSION jpg_offset = min_width;
    JDIMENSION jpg_width = width;
    JDIMENSION skip_lines = min_height;

    jpeg_crop_scanline (&cinfo, &jpg_offset, &jpg_width);
    jpeg_skip_scanlines(&cinfo, skip_lines);

    // Getting buffer ready to read data...
    unsigned long bmp_size;
    unsigned char *bmp_buffer;
    bmp_size = width * height * pixel_size;
    bmp_buffer = (unsigned char*) malloc(bmp_size);
    int row_stride = width * pixel_size; // Bytes required to fill an entire row...

    // Now we are going to read decompressed raw RGB data into buffer...
    while (cinfo.output_scanline < max_height) {
        unsigned char *buffer_array[1];
	    buffer_array[0] = bmp_buffer + (cinfo.output_scanline) * row_stride;
        jpeg_read_scanlines(&cinfo, buffer_array, 1);
    }

    // All done reading!! Time to destroy allocated buffers and what not...
    jpeg_finish_decompress(&cinfo);
    jpeg_destroy_decompress(&cinfo);

    /*
    printf("Center -> (%d, %d) :Checking row (%d) and col (%d) size, %d, %d, %d, %d\n", cent_x, cent_y, tot_width, tot_height, min_width, max_width, min_height, max_height);

    unsigned long crop_size;
    unsigned char *crop_buf;
    crop_size = d_width * d_height * pixel_size;
    crop_buf = (unsigned char*) malloc(crop_size);

    int buf_cnt = 0;
    for( int row = min_height; row < max_height; row++){
        for( int col = min_width; col < max_width; col++){
	    //printf("%d, ", col);
	    // Getting the R, G, and B values for each desired pixel ;)
	    crop_buf[buf_cnt]     = bmp_buffer[ (row_stride * row)  + col];
	    crop_buf[buf_cnt + 1] = bmp_buffer[ (row_stride * row)  + col + 1];
	    crop_buf[buf_cnt + 2] = bmp_buffer[ (row_stride * row)  + col + 2];
            buf_cnt = buf_cnt + 3;
        }
        //printf("\n");
    }
    */

    // Moment of truth, turning this into a Mat object for opencv processing...
    Mat good_img;
    Mat imgbuf(height, width, CV_8UC3, (void *)bmp_buffer);
    cvtColor(imgbuf, good_img, COLOR_RGB2BGR);

    imwrite("/stream/pic.jpg", good_img);

    free(bmp_buffer);
}

static int read_frame(void)
{
        struct v4l2_buffer buf;

        CLEAR(buf);

        buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        buf.memory = V4L2_MEMORY_MMAP;

        if (-1 == xioctl(fd, VIDIOC_DQBUF, &buf)) {
                switch (errno) {
                case EAGAIN:
                        return 0;

                case EIO:
                        /* Could ignore EIO, see spec. */

                        /* fall through */

                default:
                        errno_exit("VIDIOC_DQBUF");
                }
        }

        assert(buf.index < n_buffers);

        process_image(buffers[buf.index].start, buf.bytesused);

        if (-1 == xioctl(fd, VIDIOC_QBUF, &buf))
                errno_exit("VIDIOC_QBUF");

        return 1;
}

static void mainloop(void)
{
        unsigned int count;

        count = frame_count;

        while (count-- > 0) {
                for (;;) {
                        fd_set fds;
                        struct timeval tv;
                        int r;

                        FD_ZERO(&fds);
                        FD_SET(fd, &fds);

                        /* Timeout. */
                        tv.tv_sec = 5;
                        tv.tv_usec = 0;

                        r = select(fd + 1, &fds, NULL, NULL, &tv);

                        if (-1 == r) {
                                if (EINTR == errno)
                                        continue;
                                errno_exit("select");
                        }

                        if (0 == r) {
                                fprintf(stderr, "select timeout\n");
                                exit(EXIT_FAILURE);
                        }

                        if (read_frame())
                                break;
                        /* EAGAIN - continue select loop. */
                }
		        // Delay 40 milliseconds makes 25fps
		        // delay(10);
        }
}

static void start_capturing(void)
{
        enum v4l2_buf_type type;

        for (unsigned int i = 0; i < n_buffers; ++i) {
                struct v4l2_buffer buf;

                CLEAR(buf);
                buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
                buf.memory = V4L2_MEMORY_MMAP;
                buf.index = i;

                if (-1 == xioctl(fd, VIDIOC_QBUF, &buf))
                        errno_exit("VIDIOC_QBUF");
        }

        type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        if (-1 == xioctl(fd, VIDIOC_STREAMON, &type))
                errno_exit("VIDIOC_STREAMON");
}

static void uninit_device(void)
{
        // Freeing up the buffer and all its sub buffers...
        for (unsigned int i = 0; i < n_buffers; ++i)
                if (-1 == munmap(buffers[i].start, buffers[i].length))
                        errno_exit("munmap");
        free(buffers);
}

static void init_read(unsigned int buffer_size)
{
        buffers = (buffer *)calloc(1, sizeof(*buffers));

        if (!buffers) {
                fprintf(stderr, "Out of memory\n");
                exit(EXIT_FAILURE);
        }

        buffers[0].length = buffer_size;
        buffers[0].start = malloc(buffer_size);

        if (!buffers[0].start) {
                fprintf(stderr, "Out of memory\n");
                exit(EXIT_FAILURE);
        }
}

static void init_mmap(void)
{
        struct v4l2_requestbuffers req;

        CLEAR(req);

        req.count = 1;
        req.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        req.memory = V4L2_MEMORY_MMAP;

        if (-1 == xioctl(fd, VIDIOC_REQBUFS, &req)) {
                if (EINVAL == errno) {
                        fprintf(stderr, "%s does not support "
                                 "memory mappingn", dev_name);
                        exit(EXIT_FAILURE);
                } else {
                        errno_exit("VIDIOC_REQBUFS");
                }
        }

        if (req.count < 1) {
                fprintf(stderr, "Insufficient buffer memory on %s\n",
                         dev_name);
                exit(EXIT_FAILURE);
        }

        buffers = (buffer *)calloc(req.count, sizeof(*buffers));

        if (!buffers) {
                fprintf(stderr, "Out of memory\n");
                exit(EXIT_FAILURE);
        }

        for (n_buffers = 0; n_buffers < req.count; ++n_buffers) {
                struct v4l2_buffer buf;

                CLEAR(buf);

                buf.type        = V4L2_BUF_TYPE_VIDEO_CAPTURE;
                buf.memory      = V4L2_MEMORY_MMAP;
                buf.index       = n_buffers;

                if (-1 == xioctl(fd, VIDIOC_QUERYBUF, &buf))
                        errno_exit("VIDIOC_QUERYBUF");

                buffers[n_buffers].length = buf.length;
                buffers[n_buffers].start =
                        mmap(NULL /* start anywhere */,
                              buf.length,
                              PROT_READ | PROT_WRITE /* required */,
                              MAP_SHARED /* recommended */,
                              fd, buf.m.offset);

                if (MAP_FAILED == buffers[n_buffers].start)
                        errno_exit("mmap");
        }
}

static void init_device(void)
{

        struct v4l2_capability cap;
        struct v4l2_cropcap cropcap;
        struct v4l2_crop crop;
        struct v4l2_format fmt;
        unsigned int min;

        if (-1 == xioctl(fd, VIDIOC_QUERYCAP, &cap)) {
                if (EINVAL == errno) {
                        fprintf(stderr, "%s is no V4L2 device\n",
                                 dev_name);
                        exit(EXIT_FAILURE);
                } else {
                        errno_exit("VIDIOC_QUERYCAP");
                }
        }

        if (!(cap.capabilities & V4L2_CAP_VIDEO_CAPTURE)) {
                fprintf(stderr, "%s is no video capture device\n",
                         dev_name);
                exit(EXIT_FAILURE);
        }


        /* Select video input, video standard and tune here. */


        CLEAR(cropcap);

        cropcap.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

        if (0 == xioctl(fd, VIDIOC_CROPCAP, &cropcap)) {
                crop.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
                crop.c = cropcap.bounds; /* reset to default */

                if (-1 == xioctl(fd, VIDIOC_S_CROP, &crop)) {
                        switch (errno) {
                        case EINVAL:
                                /* Cropping not supported. */
                                printf("Cropping not supported!!\n");
				break;
                        default:
                                /* Errors ignored. */
				printf("Default error occured?\n");
                                break;
                        }
                }
        } else {
                printf("Error querying crop capabilities...\n");
        }


        CLEAR(fmt);

        fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        if (force_format) {
                fmt.fmt.pix.width       = pic_width;
                fmt.fmt.pix.height      = pic_height;
                fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_MJPEG;
                fmt.fmt.pix.field       = V4L2_FIELD_INTERLACED;

                if (-1 == xioctl(fd, VIDIOC_S_FMT, &fmt))
                        errno_exit("VIDIOC_S_FMT");

                // Note VIDIOC_S_FMT may change width and height.
        } else {
                // Preserve original settings as set by v4l2-ctl for example
                if (-1 == xioctl(fd, VIDIOC_G_FMT, &fmt))
                        errno_exit("VIDIOC_G_FMT");
        }

        // Buggy driver paranoia
        min = fmt.fmt.pix.width * 2;
        if (fmt.fmt.pix.bytesperline < min)
                fmt.fmt.pix.bytesperline = min;
        min = fmt.fmt.pix.bytesperline * fmt.fmt.pix.height;
        if (fmt.fmt.pix.sizeimage < min)
                fmt.fmt.pix.sizeimage = min;

        init_mmap();
}

static void close_device(void)
{
        if (-1 == close(fd))
                errno_exit("close");

        fd = -1;
}

static void open_device(void)
{
        struct stat st;

        if (-1 == stat(dev_name, &st)) {
                fprintf(stderr, "Cannot identify '%s': %d, %s\n",
                         dev_name, errno, strerror(errno));
                exit(EXIT_FAILURE);
        }

        if (!S_ISCHR(st.st_mode)) {
                fprintf(stderr, "%s is no devicen", dev_name);
                exit(EXIT_FAILURE);
        }

        fd = open(dev_name, O_RDWR /* required */ | O_NONBLOCK, 0);

        if (-1 == fd) {
                fprintf(stderr, "Cannot open '%s': %d, %s\n",
                         dev_name, errno, strerror(errno));
                exit(EXIT_FAILURE);
        }
}

static void usage(FILE *fp, int argc, char **argv)
{
        fprintf(fp,
                 "Usage: %s [options]\n\n"
                 "Version 1.3\n"
                 "Options:\n"
                 "-d | --device name   Video device name [%s]\n"
                 "-h | --help          Print this message\n"
                 "-m | --mmap          Use memory mapped buffers [default]\n"
                 "-o | --output        Outputs stream to stdout\n"
                 "-f | --format        Force format to 640x480 MJPEG\n"
                 "-c | --count         Number of frames to grab [%i]\n"
                 "",
                 argv[0], dev_name, frame_count);
}

static const char short_options[] = "d:hmofc:";

static const struct option
long_options[] = {
        { "device", required_argument, NULL, 'd' },
        { "help",   no_argument,       NULL, 'h' },
        { "mmap",   no_argument,       NULL, 'm' },
        { "output", no_argument,       NULL, 'o' },
        { "format", no_argument,       NULL, 'f' },
        { "count",  required_argument, NULL, 'c' },
        { 0, 0, 0, 0 }
};

int main(int argc, char **argv)
{
        dev_name = "/dev/video0";

        for (;;) {
                int idx;
                int c;

                c = getopt_long(argc, argv,
                                short_options, long_options, &idx);

                if (-1 == c)
                        break;

                switch (c) {
                case 0: /* getopt_long() flag */
                        break;

                case 'd':
                        dev_name = optarg;
                        break;

                case 'h':
                        usage(stdout, argc, argv);
                        exit(EXIT_SUCCESS);

                case 'm':
                        io = IO_METHOD_MMAP;
                        break;

                case 'o':
                        out_buf++;
                        break;

                case 'f':
                        force_format++;
                        break;

                case 'c':
                        errno = 0;
                        frame_count = strtol(optarg, NULL, 0);
                        if (errno)
                                errno_exit(optarg);
                        break;

                default:
                        usage(stderr, argc, argv);
                        exit(EXIT_FAILURE);
                }
        }

        open_device();
        init_device();
        start_capturing();
        mainloop();
        uninit_device();
        close_device();
        fprintf(stderr, "\n");
        return 0;
}
