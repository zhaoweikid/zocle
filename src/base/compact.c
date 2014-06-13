#ifdef _WIN32
#include <zocle/base/compact.h>
#include <zocle/base/defines.h>
#include <winsock2.h>
#include <ws2tcpip.h>


int fsync(int fd) {
    return _commit(fd);
}

char *strerror_r(int err, char *buf, int buflen)
{
    memset(buf, 0, buflen);
    strncpy(buf, strerror(err), buflen-1);
    return buf;
}

struct tm * 
localtime_r (const time_t *timer, struct tm *result) 
{ 
   struct tm *local; 
   local = localtime (timer); 

   if (local == NULL) 
        return NULL; 

   /*printf("1: %d-%d-%d %d:%d:%d", local->tm_year-100, local->tm_mon+1, local->tm_mday,
                local->tm_hour, local->tm_min, local->tm_sec);*/
   memcpy (result, local, sizeof (struct tm)); 

   /*printf("2: %d-%d-%d %d:%d:%d", result->tm_year-100, result->tm_mon+1, result->tm_mday,
                result->tm_hour, result->tm_min, result->tm_sec);*/
   return result; 
} 

struct tm * 
gmtime_r (const time_t *timer, struct tm *result) 
{ 
   struct tm *local_result; 
   local_result = gmtime (timer); 

   if (local_result == NULL || result == NULL) 
        return NULL; 

   memcpy (result, local_result, sizeof (result)); 
   return result; 
} 

int vasprintf( char **sptr, const char *fmt, va_list argv )
{
    int wanted = vsnprintf( *sptr = NULL, 0, fmt, argv );
    if ((wanted < 0) || ((*sptr = malloc( 1 + wanted )) == NULL))
        return -1;

    return vsprintf( *sptr, fmt, argv );
}

int asprintf( char **sptr, const char *fmt, ... )
{
    int retval;
    va_list argv;
    va_start( argv, fmt );
    retval = vasprintf( sptr, fmt, argv );
    va_end( argv );
    return retval;
}


const char * strp_weekdays[] = 
    { "sunday", "monday", "tuesday", "wednesday", "thursday", "friday", "saturday"};

const char * strp_monthnames[] = 
    { "january", "february", "march", "april", "may", "june", 
      "july", "august", "september", "october", "november", "december"};

bool strp_atoi(const char *s, int *result, int low, int high, int offset)
{
    bool worked = false;
    char * end;
    unsigned long num = strtoul(s, & end, 10);
    if (num >= (unsigned long)low && num <= (unsigned long)high)
    {
        *result = (int)(num + offset);
        s = end;
        worked = true;
    }
    return worked;
}
char * strptime(const char *s, const char *format, struct tm *tm)
{
    bool working = true;
    while (working && *format && *s)
        {
        switch (*format)
            {
        case '%':
            {
            ++format;
            switch (*format)
                {
            case 'a':
            case 'A': // weekday name
                tm->tm_wday = -1;
                working = false;
                for (size_t i = 0; i < 7; ++ i)
                    {
                    size_t len = strlen(strp_weekdays[i]);
                    if (!strnicmp(strp_weekdays[i], s, len))
                        {
                        tm->tm_wday = i;
                        s += len;
                        working = true;
                        break;
                        }
                    else if (!strnicmp(strp_weekdays[i], s, 3))
                        {
                        tm->tm_wday = i;
                        s += 3;
                        working = true;
                        break;
                        }
                    }
                break;
            case 'b':
            case 'B':
            case 'h': // month name
                tm->tm_mon = -1;
                working = false;
                for (size_t i = 0; i < 12; ++ i)
                    {
                    size_t len = strlen(strp_monthnames[i]);
                    if (!strnicmp(strp_monthnames[i], s, len))
                        {
                        tm->tm_mon = i;
                        s += len;
                        working = true;
                        break;
                        }
                    else if (!strnicmp(strp_monthnames[i], s, 3))
                        {
                        tm->tm_mon = i;
                        s += 3;
                        working = true;
                        break;
                        }
                    }
                break;
            case 'd':
            case 'e': // day of month number
                working = strp_atoi(s, &tm->tm_mday, 1, 31, -1);
                break;
            case 'D': // %m/%d/%y
                {
                const char * s_save = s;
                working = strp_atoi(s, &tm->tm_mon, 1, 12, -1);
                if (working && *s == '/')
                    {
                    ++ s;
                    working = strp_atoi(s, &tm->tm_mday, 1, 31, -1);
                    if (working && *s == '/')
                        {
                        ++ s;
                        working = strp_atoi(s, &tm->tm_year, 0, 99, 0);
                        if (working && tm->tm_year < 69)
                            tm->tm_year += 100;
                        }
                    }
                if (!working)
                    s = s_save;
                }
                break;
            case 'H': // hour
                working = strp_atoi(s, &tm->tm_hour, 0, 23, 0);
                break;
            case 'I': // hour 12-hour clock
                working = strp_atoi(s, &tm->tm_hour, 1, 12, 0);
                break;
            case 'j': // day number of year
                working = strp_atoi(s, &tm->tm_yday, 1, 366, -1);
                break;
            case 'm': // month number
                working = strp_atoi(s, &tm->tm_mon, 1, 12, -1);
                break;
            case 'M': // minute
                working = strp_atoi(s, &tm->tm_min, 0, 59, 0);
                break;
            case 'n': // arbitrary whitespace
            case 't':
                while (isspace((int)*s)) 
                    ++s;
                break;
            case 'p': // am / pm
                if (!strnicmp(s, "am", 2))
                    { // the hour will be 1 -> 12 maps to 12 am, 1 am .. 11 am, 12 noon 12 pm .. 11 pm
                    if (tm->tm_hour == 12) // 12 am == 00 hours
                        tm->tm_hour = 0;
                    }
                else if (!strnicmp(s, "pm", 2))
                    {
                    if (tm->tm_hour < 12) // 12 pm == 12 hours
                        tm->tm_hour += 12; // 1 pm -> 13 hours, 11 pm -> 23 hours
                    }
                else
                    working = false;
                break;
            case 'r': // 12 hour clock %I:%M:%S %p
                {
                const char * s_save = s;
                working = strp_atoi(s, &tm->tm_hour, 1, 12, 0);
                if (working && *s == ':')
                    {
                    ++ s;
                    working = strp_atoi(s, &tm->tm_min, 0, 59, 0);
                    if (working && *s == ':')
                        {
                        ++ s;
                        working = strp_atoi(s, &tm->tm_sec, 0, 60, 0);
                        if (working && isspace((int)*s))
                            {
                            ++ s;
                            while (isspace((int)*s)) 
                                ++s;
                            if (!strnicmp(s, "am", 2))
                                { // the hour will be 1 -> 12 maps to 12 am, 1 am .. 11 am, 12 noon 12 pm .. 11 pm
                                if (tm->tm_hour == 12) // 12 am == 00 hours
                                    tm->tm_hour = 0;
                                }
                            else if (!strnicmp(s, "pm", 2))
                                {
                                if (tm->tm_hour < 12) // 12 pm == 12 hours
                                    tm->tm_hour += 12; // 1 pm -> 13 hours, 11 pm -> 23 hours
                                }
                            else
                                working = false;
                            }
                        }
                    }
                if (!working)
                    s = s_save;
                }
                break;
            case 'R': // %H:%M
                {
                const char * s_save = s;
                working = strp_atoi(s, &tm->tm_hour, 0, 23, 0);
                if (working && *s == ':')
                    {
                    ++ s;
                    working = strp_atoi(s, &tm->tm_min, 0, 59, 0);
                    }
                if (!working)
                    s = s_save;
                }
                break;
            case 'S': // seconds
                working = strp_atoi(s, &tm->tm_sec, 0, 60, 0);
                break;
            case 'T': // %H:%M:%S
                {
                const char * s_save = s;
                working = strp_atoi(s, &tm->tm_hour, 0, 23, 0);
                if (working && *s == ':')
                    {
                    ++ s;
                    working = strp_atoi(s, &tm->tm_min, 0, 59, 0);
                    if (working && *s == ':')
                        {
                        ++ s;
                        working = strp_atoi(s, &tm->tm_sec, 0, 60, 0);
                        }
                    }
                if (!working)
                    s = s_save;
                }
                break;
            case 'w': // weekday number 0->6 sunday->saturday
                working = strp_atoi(s, &tm->tm_wday, 0, 6, 0);
                break;
            case 'Y': // year
                working = strp_atoi(s, &tm->tm_year, 1900, 65535, -1900);
                break;
            case 'y': // 2-digit year
                working = strp_atoi(s, &tm->tm_year, 0, 99, 0);
                if (working && tm->tm_year < 69)
                    tm->tm_year += 100;
                break;
            case '%': // escaped
                if (*s != '%')
                    working = false;
                ++s;
                break;
            default:
                working = false;
                }
            }
            break;
        case ' ':
        case '\t':
        case '\r':
        case '\n':
        case '\f':
        case '\v':
            // zero or more whitespaces:
            while (isspace((int)*s))
                ++ s;
            break;
        default:
            // match character
            if (*s != *format)
                working = false;
            else
                ++s;
            break;
            }
        ++format;
        }
    return (working?(char *)s:0);
}

/*const char *inet_ntop(int af, const void *src, char *dst, socklen_t cnt) 
{ 
    if (af == AF_INET) { 
        struct sockaddr_in in; 
        memset(&in, 0, sizeof(in)); 
        in.sin_family = AF_INET; 
        memcpy(&in.sin_addr, src, sizeof(struct in_addr)); 
        getnameinfo((struct sockaddr *)&in, sizeof(struct sockaddr_in), dst, cnt, NULL, 0, NI_NUMERICHOST); 
                return dst; 
    } else if (af == AF_INET6) { 
        struct sockaddr_in6 in; 
        memset(&in, 0, sizeof(in)); 
        in.sin6_family = AF_INET6; 
        memcpy(&in.sin6_addr, src, sizeof(struct in_addr6)); 
        getnameinfo((struct sockaddr *)&in, sizeof(struct sockaddr_in6), dst, cnt, NULL, 0, NI_NUMERICHOST); 
        return dst; 
    } 
    return NULL; 
} 

int inet_pton(int af, const char *src, void *dst) 
{ 
    struct addrinfo hints, *res, *ressave; 

    memset(&hints, 0, sizeof(struct addrinfo)); 
    hints.ai_family = af; 

    if (getaddrinfo(src, NULL, &hints, &res) != 0) { 
            //dolog(LOG_ERR, "Couldn't resolve host %s\n", src); 
        return -1; 
    } 
    ressave = res; 
    while (res) { 
        memcpy(dst, res->ai_addr, res->ai_addrlen); 
        res = res->ai_next; 
    } 

    freeaddrinfo(ressave); 
    return 0; 
} */


#endif

