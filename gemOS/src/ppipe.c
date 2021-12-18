#include <ppipe.h>
#include <context.h>
#include <memory.h>
#include <lib.h>
#include <entry.h>
#include <file.h>

int min(int a, int b)
{
    if (a < b)
        return a;
    else
        return b;
}
int max(int a, int b)
{
    if (a > b)
        return a;
    else
        return b;
}
// Per process information for the ppipe.
struct ppipe_info_per_process
{

    // TODO:: Add members as per your need...
    int read;
    int pid;
    int open_read;
    int open_write;
    int total_data;
};

// Global information for the ppipe.
struct ppipe_info_global
{

    char *ppipe_buff; // Persistent pipe buffer: DO NOT MODIFY THIS.

    // TODO:: Add members as per your need...
    int flush_upto, write;
    int total_processes;
    int overall_data;
};

// Persistent pipe structure.
// NOTE: DO NOT MODIFY THIS STRUCTURE.
struct ppipe_info
{

    struct ppipe_info_per_process ppipe_per_proc[MAX_PPIPE_PROC];
    struct ppipe_info_global ppipe_global;
};

// Function to allocate space for the ppipe and initialize its members.
struct ppipe_info *alloc_ppipe_info()
{

    // Allocate space for ppipe structure and ppipe buffer.
    struct ppipe_info *ppipe = (struct ppipe_info *)os_page_alloc(OS_DS_REG);
    char *buffer = (char *)os_page_alloc(OS_DS_REG);

    // Assign ppipe buffer.
    ppipe->ppipe_global.ppipe_buff = buffer;

    /**
     *  TODO:: Initializing pipe fields
     *
     *  Initialize per process fields for this ppipe.
     *  Initialize global fields for this ppipe.
     *
     */
    u32 pid = get_current_ctx()->pid;
    ppipe->ppipe_global.flush_upto = 0;
    ppipe->ppipe_global.write = 0;
    ppipe->ppipe_per_proc[0].pid = pid;
    ppipe->ppipe_per_proc[0].open_read = 1;
    ppipe->ppipe_per_proc[0].open_write = 1;
    ppipe->ppipe_per_proc[0].read = 0;
    ppipe->ppipe_global.total_processes = 0;
    ppipe->ppipe_per_proc[0].total_data=0;
    ppipe->ppipe_global.overall_data=MAX_PPIPE_SIZE;
    for (int i = 1; i < MAX_PPIPE_PROC; i++)
    {
        ppipe->ppipe_per_proc[i].pid = -1;
    }
    // Return the ppipe.
    return ppipe;
}

// Function to free ppipe buffer and ppipe info object.
// NOTE: DO NOT MODIFY THIS FUNCTION.
void free_ppipe(struct file *filep)
{
    os_page_free(OS_DS_REG, filep->ppipe->ppipe_global.ppipe_buff);
    os_page_free(OS_DS_REG, filep->ppipe);
}

// Fork handler for ppipe.
int do_ppipe_fork(struct exec_context *child, struct file *filep)
{

    /**
     *  TODO:: Implementation for fork handler
     *
     *  You may need to update some per process or global info for the ppipe.
     *  This handler will be called twice since ppipe has 2 file objects.
     *  Also consider the limit on no of processes a ppipe can have.
     *  Return 0 on success.
     *  Incase of any error return -EOTHERS.
     *
     */
    // u32 p_pid=child->ppid;
    // u32 c_pid=child->pid;
    int retvalue;
    if (filep == NULL)
        return 0;
    u32 c_pid = child->pid;
    u32 p_pid = get_current_ctx()->pid;
    int x = 0;
    int c_index = -1;
    for (int i = 0; i < MAX_PPIPE_PROC; i++)
    {
        if (filep->ppipe->ppipe_per_proc[i].pid == c_pid)
        {
            x = 1;
            c_index = i;
            break;
        }
    }
    if (x == 1)
    {
        for (int i = 0; i < MAX_PPIPE_PROC; i++)
        {
            if (filep->ppipe->ppipe_per_proc[i].pid == p_pid)
            {
                if (filep->mode & O_READ)
                {
                    filep->ppipe->ppipe_per_proc[c_index].open_read = filep->ppipe->ppipe_per_proc[i].open_read;
                    filep->ppipe->ppipe_per_proc[c_index].read = filep->ppipe->ppipe_per_proc[i].read;
                    filep->ppipe->ppipe_per_proc[c_index].total_data=filep->ppipe->ppipe_per_proc[i].total_data;
                    retvalue = 0;
                }
                else if (filep->mode & O_WRITE)
                {
                    filep->ppipe->ppipe_per_proc[c_index].open_write = filep->ppipe->ppipe_per_proc[i].open_write;
                    filep->ppipe->ppipe_per_proc[c_index].total_data=filep->ppipe->ppipe_per_proc[i].total_data;
                    retvalue = 0;
                }
                else
                    retvalue = -EOTHERS;
            }
        }
    }
    else
    {
        for (int i = 0; i < MAX_PPIPE_PROC; i++)
        {
            if (filep->ppipe->ppipe_per_proc[i].pid == -1)
            {
                filep->ppipe->ppipe_global.total_processes++;
                c_index = i;
                filep->ppipe->ppipe_per_proc[i].pid = c_pid;
                filep->ppipe->ppipe_per_proc[i].open_read = 0;
                filep->ppipe->ppipe_per_proc[i].open_write = 0;
                filep->ppipe->ppipe_per_proc[c_index].total_data=filep->ppipe->ppipe_per_proc[i].total_data;
                break;
            }
        }
        printk("%d\n", c_index);
        if (c_index == -1)
        {
            return -EOTHERS;
        }
        for (int i = 0; i < MAX_PPIPE_PROC; i++)
        {
            if (p_pid == filep->ppipe->ppipe_per_proc[i].pid)
            {
                if (filep->mode & O_READ)
                {
                    filep->ppipe->ppipe_per_proc[c_index].open_read = filep->ppipe->ppipe_per_proc[i].open_read;
                    filep->ppipe->ppipe_per_proc[c_index].read = filep->ppipe->ppipe_per_proc[i].read;
                    retvalue = 0;
                }
                else if (filep->mode & O_WRITE)
                {
                    filep->ppipe->ppipe_per_proc[c_index].open_write = filep->ppipe->ppipe_per_proc[i].open_write;
                    retvalue = 0;
                }
                else
                    return -EOTHERS;
            }
        }
    }
    // Return successfully.
    return 0;
}

// Function to close the ppipe ends and free the ppipe when necessary.
long ppipe_close(struct file *filep)
{

    /**
     *  TODO:: Implementation of Pipe Close
     *
     *  Close the read or write end of the ppipe depending upon the file
     *      object's mode.
     *  You may need to update some per process or global info for the ppipe.
     *  Use free_pipe() function to free ppipe buffer and ppipe object,
     *      whenever applicable.
     *  After successful close, it return 0.
     *  Incase of any error return -EOTHERS.
     *                                                                          
     */

    int ret_value;
    if (filep == NULL)
        return 0;
    u32 pid = get_current_ctx()->pid;
    int p_index;
    for (p_index = 0; p_index < MAX_PPIPE_PROC; p_index++)
    {
        if (filep->ppipe->ppipe_per_proc[p_index].pid == pid)
            break;
    }
    if ((filep->mode) & O_READ)
    {
        filep->ppipe->ppipe_per_proc[p_index].open_read = 0;
    }
    else if ((filep->mode) & O_WRITE)
    {
        filep->ppipe->ppipe_per_proc[p_index].open_write = 0;
    }
    else
        return -EOTHERS;
    int r = filep->ppipe->ppipe_per_proc[p_index].open_read;
    int w = filep->ppipe->ppipe_per_proc[p_index].open_write;
    if (r == 0 && w == 0)
    {
        filep->ppipe->ppipe_global.total_processes--;
        filep->ppipe->ppipe_per_proc[p_index].pid = -1;
    }
    if (filep->ppipe->ppipe_global.total_processes == 0)
    {
        free_ppipe(filep);
    }

    // Close the file.
    ret_value = file_close(filep); // DO NOT MODIFY THIS LINE.

    // And return.
    return ret_value;
}

// Function to perform flush operation on ppipe.
int do_flush_ppipe(struct file *filep)
{

    /**
     *  TODO:: Implementation of Flush system call
     *
     *  Reclaim the region of the persistent pipe which has been read by 
     *      all the processes.
     *  Return no of reclaimed bytes.
     *  In case of any error return -EOTHERS.
     *
     */
    int all_close = 1;
    if(filep==NULL)return 0;
    int flush = filep->ppipe->ppipe_global.flush_upto;
    int min1_read_pointer = MAX_PPIPE_SIZE + 1;
    int min2_read_pointer = MAX_PPIPE_SIZE + 1;
    for (int i = 0; i < MAX_PPIPE_PROC; i++)
    {
        if (filep->ppipe->ppipe_per_proc[i].open_read == 1)
        {
            min1_read_pointer = min(min1_read_pointer, filep->ppipe->ppipe_per_proc[i].read);
            if (filep->ppipe->ppipe_per_proc[i].read >= flush)
                min2_read_pointer = min(min2_read_pointer, filep->ppipe->ppipe_per_proc[i].read);
        }
    }
    int reclaimed_bytes = 0;
    printk("F: %d %d\n",min1_read_pointer,min2_read_pointer);
    if (min2_read_pointer != MAX_PPIPE_SIZE + 1)
    {
        reclaimed_bytes = min2_read_pointer - flush;
        filep->ppipe->ppipe_global.flush_upto = min2_read_pointer-flush;
    }
    else
    {
        reclaimed_bytes = 4096 - flush + min1_read_pointer;
        filep->ppipe->ppipe_global.flush_upto = min1_read_pointer;
    }
    filep->ppipe->ppipe_global.overall_data+=reclaimed_bytes;
    printk("%d\n",filep->ppipe->ppipe_global.overall_data);
    // Return reclaimed bytes.
    return reclaimed_bytes;
}

// Read handler for the ppipe.
int ppipe_read(struct file *filep, char *buff, u32 count)
{
    /**
     *  TODO:: Implementation of PPipe Read
     *
     *  Read the data from ppipe buffer and write to the provided buffer.
     *  If count is greater than the present data size in the ppipe then just read
     *      that much data.
     *  Validate file object's access right.
     *  On successful read, return no of bytes read.
     *  Incase of Error return valid error code.
     *      -EACCES: In case access is not valid.
     *      -EINVAL: If read end is already closed.
     *      -EOTHERS: For any other errors.
     *
     */
    if ((filep->mode & O_READ) == 0)
        return -EINVAL;
    u32 pid = get_current_ctx()->pid;
    int p_index;
    for (p_index = 0; p_index < MAX_PPIPE_PROC; p_index++)
    {
        if (filep->ppipe->ppipe_per_proc[p_index].pid == pid)
            break;
    }
    if (filep->ppipe->ppipe_per_proc[p_index].open_read == 0)
        return -EINVAL;
    int ind;
    int bytes_read = 0;
    while (bytes_read < count && filep->ppipe->ppipe_per_proc[p_index].total_data>0)
    {
        ind = filep->ppipe->ppipe_per_proc[p_index].read;
        buff[bytes_read] = filep->ppipe->ppipe_global.ppipe_buff[ind];
        filep->ppipe->ppipe_per_proc[p_index].read = (ind + 1) % MAX_PPIPE_SIZE;
        filep->ppipe->ppipe_per_proc[p_index].total_data--;
        bytes_read++;
    }

    // Return no of bytes read.
    return bytes_read;
}

// Write handler for ppipe.
int ppipe_write(struct file *filep, char *buff, u32 count)
{

    /**
     *  TODO:: Implementation of PPipe Write
     *
     *  Write the data from the provided buffer to the ppipe buffer.
     *  If count is greater than available space in the ppipe then just write
     *      data that fits in that space.
     *  Validate file object's access right.
     *  On successful write, return no of written bytes.
     *  Incase of Error return valid error code.
     *      -EACCES: In case access is not valid.
     *      -EINVAL: If write end is already closed.
     *      -EOTHERS: For any other errors.
     *
     */
    int p_index = 0;
    u32 pid = get_current_ctx()->pid;
    for (p_index = 0; p_index < MAX_PPIPE_PROC; p_index++)
    {
        if (filep->ppipe->ppipe_per_proc[p_index].pid == pid)
            break;
    }
    if (filep->ppipe->ppipe_per_proc[p_index].open_write == 0)
        return -EACCES;
    int bytes_written = 0;
    int ind;

    while (bytes_written < count && filep->ppipe->ppipe_global.overall_data>0)
    {
        ind = filep->ppipe->ppipe_global.write;
        if(ind%1000==0)
        {
            printk("%d\n",ind);
        }
        filep->ppipe->ppipe_global.ppipe_buff[ind] = buff[bytes_written++];
        filep->ppipe->ppipe_global.write = (ind + 1) % MAX_PPIPE_SIZE;
        filep->ppipe->ppipe_global.overall_data--;
        for(int i=0;i<MAX_PPIPE_PROC;i++)
        {
            if(filep->ppipe->ppipe_per_proc[i].pid!=-1)
            {
                filep->ppipe->ppipe_per_proc[i].total_data++;
            }
        }
    }
    // Return no of bytes written.
    return bytes_written;
}

// Function to create persistent pipe.
int create_persistent_pipe(struct exec_context *current, int *fd)
{
    /**
     *  TODO:: Implementation of PPipe Create
     *
     *  Find two free file descriptors.
     *  Create two file objects for both ends by invoking the alloc_file() function.
     *  Create ppipe_info object by invoking the alloc_ppipe_info() function and
     *      fill per process and global info fields.
     *  Fill the fields for those file objects like type, fops, etc.
     *  Fill the valid file descriptor in *fd param.
     *  On success, return 0.
     *  Incase of Error return valid Error code.
     *      -ENOMEM: If memory is not enough.
     *      -EOTHERS: Some other errors.
     *
     */
    int a = -1, b = -1;
    int ret = -EOTHERS;
    for (int i = 0; i < MAX_OPEN_FILES; i++)
    {
        if (!(current->files[i]))
        {
            a = i;
            break;
        }
    }
    if (a == -1)
        return ret;
    for (int i = a + 1; i < MAX_OPEN_FILES; i++)
    {
        if (!(current->files[i]))
        {
            b = i;
            break;
        }
    }
    if (b == -1)
        return ret;
    printk("%d %d\n", a, b);
    fd[0] = a;
    fd[1] = b;
    struct file *Read = alloc_file();
    struct file *Write = alloc_file();
    struct ppipe_info *R = alloc_ppipe_info();
    Read->fops->read = ppipe_read;
    Write->fops->write = ppipe_write;
    Read->fops->close = ppipe_close;
    Write->fops->close = ppipe_close;
    Read->mode = O_READ;
    Write->mode = O_WRITE;
    Read->ppipe = R;
    Write->ppipe = R;
    Read->type = PPIPE;
    Write->type = PPIPE;
    current->files[a] = Read;
    current->files[b] = Write;

    // Simple return.
    return 0;
}

