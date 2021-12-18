#include <pipe.h>
#include <context.h>
#include <memory.h>
#include <lib.h>
#include <entry.h>
#include <file.h>
typedef unsigned long ULONG;
// Per process info for the pipe.
struct pipe_info_per_process
{

    // TODO:: Add members as per your need...
    int open_read;
    int open_write;
    int pid;
};

// Global information for the pipe.
struct pipe_info_global
{

    char *pipe_buff; // Pipe buffer: DO NOT MODIFY THIS.

    // TODO:: Add members as per your need...
    int total_processes;
    int l, r;
    int total_space;
};

// Pipe information structure.
// NOTE: DO NOT MODIFY THIS STRUCTURE.
struct pipe_info
{

    struct pipe_info_per_process pipe_per_proc[MAX_PIPE_PROC];
    struct pipe_info_global pipe_global;
};

// Function to allocate space for the pipe and initialize its members.
struct pipe_info *alloc_pipe_info()
{

    // Allocate space for pipe structure and pipe buffer.
    struct pipe_info *pipe = (struct pipe_info *)os_page_alloc(OS_DS_REG);
    char *buffer = (char *)os_page_alloc(OS_DS_REG);

    // Assign pipe buffer.
    pipe->pipe_global.pipe_buff = buffer;

    /**
     *  TODO:: Initializing pipe fields
     *  
     *  Initialize per process fields for this pipe.
     *  Initialize global fields for this pipe.
     *
     */
    pipe->pipe_global.l = 0;
    pipe->pipe_global.r = 0;
    pipe->pipe_per_proc[0].open_read = 1;
    pipe->pipe_per_proc[0].open_write = 1;
    pipe->pipe_per_proc[0].pid = get_current_ctx()->pid;
    for (int i = 1; i < MAX_PIPE_PROC; i++)
    {
        pipe->pipe_per_proc[i].pid = -1;
    }
    pipe->pipe_global.total_processes = 1;
    pipe->pipe_global.total_space=4096;
    // Return the pipe.
    return pipe;
}

// Function to free pipe buffer and pipe info object.
// NOTE: DO NOT MODIFY THIS FUNCTION.
void free_pipe(struct file *filep)
{

    os_page_free(OS_DS_REG, filep->pipe->pipe_global.pipe_buff);
    os_page_free(OS_DS_REG, filep->pipe);
}

// Fork handler for the pipe.
int do_pipe_fork(struct exec_context *child, struct file *filep)
{
    /**
     *  TODO:: Implementation for fork handler
     *
     *  You may need to update some per process or global info for the pipe.
     *  This handler will be called twice since pipe has 2 file objects.
     *  Also consider the limit on no of processes a pipe can have.
     *  Return 0 on success.
     *  Incase of any error return -EOTHERS.
     *
     */
    int retvalue;
    if (filep == NULL)
        return 0;
    u32 c_pid = child->pid;
    u32 p_pid = get_current_ctx()->pid;
    int x = 0;
    int c_index = -1;
    for (int i = 0; i < MAX_PIPE_PROC; i++)
    {
        if (filep->pipe->pipe_per_proc[i].pid == c_pid)
        {
            x = 1;
            c_index = i;
            break;
        }
    }
    if (x == 1)
    {
        filep->ref_count++;
        for (int i = 0; i < MAX_PIPE_PROC; i++)
        {
            if (filep->pipe->pipe_per_proc[i].pid == p_pid)
            {
                if (filep->mode & O_READ)
                {
                    filep->pipe->pipe_per_proc[c_index].open_read = filep->pipe->pipe_per_proc[i].open_read;
                    retvalue = 0;
                }
                else if (filep->mode & O_WRITE)
                {
                    filep->pipe->pipe_per_proc[c_index].open_write = filep->pipe->pipe_per_proc[i].open_write;
                    retvalue = 0;
                }
                else
                    retvalue = -EOTHERS;
            }
        }
    }
    else
    {
        for (int i = 0; i < MAX_PIPE_PROC; i++)
        {
            if (filep->pipe->pipe_per_proc[i].pid == -1)
            {
                filep->ref_count++;
                filep->pipe->pipe_global.total_processes++;
                c_index = i;
                filep->pipe->pipe_per_proc[i].pid = c_pid;
                filep->pipe->pipe_per_proc[i].open_read = 0;
                filep->pipe->pipe_per_proc[i].open_write = 0;
                break;
            }
        }
        printk("c_index: %d\n",c_index);
        if(c_index==-1)
        {
            return -EOTHERS;
        }
        for (int i = 0; i < MAX_PIPE_PROC; i++)
        {
            if (p_pid == filep->pipe->pipe_per_proc[i].pid)
            {
                if (filep->mode & O_READ)
                {
                    filep->pipe->pipe_per_proc[c_index].open_read = filep->pipe->pipe_per_proc[i].open_read;
                    retvalue = 0;
                }
                else if (filep->mode & O_WRITE)
                {
                    filep->pipe->pipe_per_proc[c_index].open_write = filep->pipe->pipe_per_proc[i].open_write;
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

// Function to close the pipe ends and free the pipe when necessary.
long pipe_close(struct file *filep)
{

    /**
     *  TODO:: Implementation of Pipe Close
     *
     *  Close the read or write end of the pipe depending upon the file
     *      object's mode.
     *  You may need to update some per process or global info for the pipe.
     *  Use free_pipe() function to free pipe buffer and pipe object,
     *      whenever applicable.
     *  After successful close, it return 0.
     *  Incase of any error return -EOTHERS.
     *
     */
    int ret_value = -EOTHERS;
    struct exec_context *current = get_current_ctx();
    int pid = current->pid;
    int p_index = -1;
    for (p_index = 0; p_index < MAX_PIPE_PROC; p_index++)
    {
        if (filep->pipe->pipe_per_proc[p_index].pid == pid)
            break;
    }
    if ((filep->mode) & O_READ)
    {
        filep->pipe->pipe_per_proc[p_index].open_read = 0;
    }
    else if ((filep->mode) & O_WRITE)
    {
        filep->pipe->pipe_per_proc[p_index].open_write = 0;
    }
    else
        return ret_value;
    int r = filep->pipe->pipe_per_proc[p_index].open_read;
    int w = filep->pipe->pipe_per_proc[p_index].open_write;
    if (r == 0 && w == 0)
    {
        filep->pipe->pipe_global.total_processes--;
        filep->pipe->pipe_per_proc[p_index].pid = -1;
    }
    // Close the file and return.
    if (filep->pipe->pipe_global.total_processes == 0)
    {
        free_pipe(filep);
    }
    ret_value = file_close(filep); // DO NOT MODIFY THIS LINE.

    // And return.
    return ret_value;
}

// Check whether passed buffer is valid memory location for read or write.
int is_valid_mem_range(unsigned long buff, u32 count, int access_bit)
{

    /**
     *  TODO:: Implementation for buffer memory range checking
     *
     *  Check whether passed memory range is suitable for read or write.
     *  If access_bit == 1, then it is asking to check read permission.
     *  If access_bit == 2, then it is asking to check write permission.
     *  If range is valid then return 1.
     *  Incase range is not valid or have some permission issue return -EBADMEM.
     *
     */

    int ret_value = -EBADMEM;
    struct exec_context *current = get_current_ctx();
    unsigned long start, end;
    u32 af;
    for (int i = 0; i < MAX_MM_SEGS - 1; i++)
    {
        start = current->mms[i].start, end = current->mms[i].next_free;
        af = current->mms[i].access_flags;
        if (start <= buff && end >= buff + count && (access_bit & af))
        {
            ret_value = 1;
            break;
        }
    }
    start = current->mms[MAX_MM_SEGS - 1].start;
    end = current->mms[MAX_MM_SEGS - 1].end;
    af = current->mms[MAX_MM_SEGS - 1].access_flags;
    printk("Stack: %x %x %d\n",start,end,af);
    if (start <= buff && end >= buff + count - 1)
    {
        ret_value = 1;
    }
    struct vm_area* vm=current->vm_area;
    while(vm)
    {
        start=vm->vm_start;
        end=vm->vm_end;
        af=vm->access_flags;
        printk("VM: %x %x %d\n",start,end,af);
        if(start<=buff&&end>=buff+count-1&&(af&access_bit))
        {
            return 1;
        }
        vm=vm->vm_next;
    }
    // Return the finding.
    return ret_value;
     
}

// Function to read given no of bytes from the pipe.
int pipe_read(struct file *filep, char *buff, u32 count)
{

    /**
     *  TODO:: Implementation of Pipe Read
     *
     *  Read the data from pipe buffer and write to the provided buffer.
     *  If count is greater than the present data size in the pipe then just read
     *       that much data.
     *  Validate file object's access right.
     *  On successful read, return no of bytes read.
     *  Incase of Error return valid error code.
     *       -EACCES: In case access is not valid.
     *       -EINVAL: If read end is already closed.
     *       -EOTHERS: For any other errors.
     *
     */
    if (((filep->mode) & O_READ) == 0)
    {
        return -EACCES;
    }
    int pid = get_current_ctx()->pid;
    int p_index = 0;
    for (p_index = 0; p_index < MAX_PIPE_PROC; p_index++)
    {
        if (pid == filep->pipe->pipe_per_proc[p_index].pid)
            break;
    }
    if (filep->pipe->pipe_per_proc[p_index].open_read == 0)
        return -EINVAL;
    unsigned long Buff = (unsigned long)buff;
    if (is_valid_mem_range(Buff, count, 2) < 0)
    {
        return -EOTHERS;
    }

    int bytes_read = 0;
    while (bytes_read < count && filep->pipe->pipe_global.total_space<4096)
    {
        buff[bytes_read] = filep->pipe->pipe_global.pipe_buff[filep->pipe->pipe_global.l];
        filep->pipe->pipe_global.l = (filep->pipe->pipe_global.l + 1) % MAX_PIPE_SIZE;
        bytes_read++;
        filep->pipe->pipe_global.total_space++;
    }

    // Return no of bytes read.
    return bytes_read;
}

// Function to write given no of bytes to the pipe.
int pipe_write(struct file *filep, char *buff, u32 count)
{

    /**
     *  TODO:: Implementation of Pipe Write
     *
     *  Write the data from the provided buffer to the pipe buffer.
     *  If count is greater than available space in the pipe then just write data
     *       that fits in that space.
     *  Validate file object's access right.
     *  On successful write, return no of written bytes.
     *  Incase of Error return valid error code.
     *       -EACCES: In case access is not valid.
     *       -EINVAL: If write end is already closed.
     *       -EOTHERS: For any other errors.
     *
     */
    if (((filep->mode) & O_WRITE) == 0)
    {
        return -EACCES;
    }
    int pid = get_current_ctx()->pid;
    int p_index = 0;
    for (p_index = 0; p_index < MAX_PIPE_PROC; p_index++)
    {
        if (pid == filep->pipe->pipe_per_proc[p_index].pid)
            break;
    }
    unsigned long Buff = (unsigned long)buff;
    if (filep->pipe->pipe_per_proc[p_index].open_write == 0)
        return -EINVAL;
    if (is_valid_mem_range(Buff, count, 1) < 0)
    {
        return -EOTHERS;
    }
    int bytes_written = 0;
    while (bytes_written < count && filep->pipe->pipe_global.total_space>0)
    {
        filep->pipe->pipe_global.pipe_buff[filep->pipe->pipe_global.r] = buff[bytes_written];
        bytes_written++;
        filep->pipe->pipe_global.r = (filep->pipe->pipe_global.r + 1) % MAX_PIPE_SIZE;
        filep->pipe->pipe_global.total_space--;
    }

    // Return no of bytes written.
    return bytes_written;
}

// Function to create pipe.
int create_pipe(struct exec_context *current, int *fd)
{

    /**
     *  TODO:: Implementation of Pipe Create
     *
     *  Find two free file descriptors.
     *  Create two file objects for both ends by invoking the alloc_file() function. 
     *  Create pipe_info object by invoking the alloc_pipe_info() function and
     *       fill per process and global info fields.
     *  Fill the fields for those file objects like type, fops, etc.
     *  Fill the valid file descriptor in *fd param.
     *  On success, return 0.
     *  Incase of Error return valid Error code.
     *       -ENOMEM: If memory is not enough.
     *       -EOTHERS: Some other errors.
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
    fd[0] = a;
    fd[1] = b;
    struct file *Read = alloc_file();
    struct file *Write = alloc_file();
    Read->mode = O_READ;
    Write->mode = O_WRITE;
    Read->type = PIPE;
    Write->type = PIPE;
    current->files[a] = Read;
    current->files[b] = Write;
    Read->fops->read = pipe_read;
    Read->fops->close = pipe_close;
    Write->fops->write = pipe_write;
    Write->fops->close = pipe_close;
    current->files[a] = Read;
    current->files[b] = Write;
    struct pipe_info *R1 = alloc_pipe_info();
    current->files[a]->pipe = R1;
    current->files[b]->pipe = R1;

    return 0;
}

