#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/ioctl.h>

#define ERR(s) err(s,__FILE__,__LINE__)

static void err(char *s, char *file, int line) {
  fprintf(stderr,"%s:%d: %s\n",file,line,s);
  exit(1);
}

// Test 1: Default separators
// It uses space, tab, newline, and colon as default separators.
void test1_default_separators() {
  printf("Test 1: Default Separators\n");
  int fd=open("/dev/scanner",O_RDWR); // open device
  if (fd<0){ 
    ERR("open() failed");
  }
  const char *data="hello:world\tthis is \na test"; // input data
  printf("input data: \"%s\"\n",data);
  printf("separators: space, tab, newline, colon\n");

  if (write(fd,data,strlen(data))<0) // write data to device
    ERR("write() failed");
  
  // Read tokens
  char buf[128]; // buffer for token
  int token=0; // token counter
  int len; // length of read data

  // This loop reads tokens until no more tokens are available
  while (1) {
    len=read(fd,buf,sizeof(buf)); // read token from device
    if (len>0) { // we got token data
      buf[len]=0; // null-terminate the string
      printf("  Token %d: \"%s\" (%d bytes)\n",token,buf,len);
    } else if (len==0) { // end of token
      printf("   End of Token %d\n",token);
      token++;
    } else { // end of data (-1)
      printf("End of data\n");
      break;
    }
  }
  printf("Total tokens: %d\n",token);
  close(fd);
}

// Test 2: Custom separators via ioctl
// custom separators '-' and ','
void test2_custom_separators() {
  printf("Test 2: Custom Separators\n");
  int fd=open("/dev/scanner",O_RDWR); // open device
  if (fd<0){
    ERR("open() failed");
  }

  // Switch to configuration mode
  if (ioctl(fd,0,0)<0) // 
    ERR("ioctl() failed"); 

  // Set custom separators
  const char *custom_separators="-,";
  // if write fails
  if (write(fd,custom_separators,strlen(custom_separators))<0)
    ERR("write() failed to set custom separators");
  
  // Write data with custom separators
  const char *data="hello-world,miguel-carrasco";
  printf("input data: \"%s\"\n",data);
  printf("separators: '-', ','\n");

  if (write(fd,data,strlen(data))<0) // write data to device
    ERR("write() failed"); 
  
  // Read tokens
  char buf[128]; // buffer for token
  int token=0; // token counter
  int len; // length of read data

  // This loop reads tokens until no more tokens are available
  while (1) {
    len=read(fd,buf,sizeof(buf)); // read token from device
    if (len>0) { // we got token data
      buf[len]=0; // null-terminate the string
      printf("  Token %d: \"%s\" (%d bytes)\n",token,buf,len);
    } else if (len==0) { // end of token
      printf("   End of Token %d\n",token);
      token++;
    } else { // end of data (-1)
      printf("End of data\n");
      break;
    }
  }
  printf("Total tokens: %d\n",token);
  close(fd);
}

// Test 3: Non-cumulative writes
// Each write replaces previous data
void test3_non_cumulative_writes() {
  printf("Test 3: Non-Cumulative Writes\n");
  int fd=open("/dev/scanner",O_RDWR); // open device
  if (fd<0){
    ERR("open() failed");
  }

  // First write
  const char *data1="first:test";
  if (write(fd,data1,strlen(data1))<0) // write data to device
    ERR("first write() failed");

  // second write
  const char *data2="second:test";
  if (write(fd,data2,strlen(data2))<0) // write data to device
    ERR("second write() failed");

  // ingore first data from first write
  printf("input data should be \"%s\"\n",data2);
  const char *expected[] = { "second", "test" };
  int expected_tokens = 2;

  // Read tokens
  char buf[128]; // buffer for token
  int token=0; // token counter
  int len; // length of read data
  int pass = 1; // flag to track test pass/fail
  // This loop reads tokens until no more tokens are available
  while (1) {
    len=read(fd,buf,sizeof(buf)); // read token from device
    if (len>0) { // we got token data
      buf[len]=0; // null-terminate the string
      printf("  Token %d: \"%s\" (%d bytes)\n",token,buf,len);
    
    if (token >= expected_tokens || strcmp(buf, expected[token]) != 0) {
        pass = 0; // unexpected token
      }
    } else if (len==0) { // end of token
      printf("   End of Token %d\n",token);
      token++;
    } else { // end of data (-1)
      printf("End of data\n");
      break;
    }
  }
  printf("Total tokens: %d\n",token);
  if (token != expected_tokens) {
      pass = 0; // incorrect number of tokens
    }
  if (pass) {
    printf("Test 3 result: PASS\n");
  } else {
    printf("Test 3 result: FAIL\n");
  }
    close(fd);
}

// Test 4: Partial reads (token larger than buffer)
void test4_partial_reads() {
  printf("Test 4: Partial Reads\n");
  int fd=open("/dev/scanner",O_RDWR); // open device
  if (fd<0){
    ERR("open() failed");
  }

  const char *data="verylongtoken:short";
  if (write(fd,data,strlen(data))<0) // write data to device
    ERR("write() failed");
  
  printf("input data: \"%s\"\n",data);
  printf("buffer size: 4 bytes\n");

  char buf[5]; // 4 bytes + null terminator
  int len; // length of read data
  int token=0; // token counter
  int chunk=0; // chunk counter within token
  int passed=1; // flag to track test pass/fail

  const char *expected_chunks[] = {
    "very", "long", "toke", "n", // first token chunks
    "shor", "t",
    NULL                 // second token chunks
  };
  while (1) {
    len = read(fd, buf, 4); // read up to 4 bytes
    if (len > 0) {
      buf[len] = 0; // null-terminate
      printf("  Chunk %d: \"%s\" (%d bytes)\n", chunk, buf, len);
      if (expected_chunks[chunk] == NULL || strcmp(buf, expected_chunks[chunk]) != 0)
        passed = 0; // unexpected chunk
      
      chunk++; // next chunk
    }
    else if (len == 0) {
      printf("   End of Token %d\n", token);
      token++;
      
    } else {
      printf("End of data\n");
      break;
    }
  }
  if (passed) {
    printf("Test 4 result: PASS\n");
  } else {
    printf("Test 4 result: FAIL\n");
  }
  close(fd);
}

// Test 5: Handling NUL bytes in data
void test5_nul_bytes() {
  printf("Test 5: NUL Byte Handling\n");
  int fd=open("/dev/scanner",O_RDWR); // open device
  if (fd<0){
    ERR("open() failed");
  }

  char data_with_nul[] = {'h','e','l','\0','l','o',':','w','o','r','l','d'};
  printf("input data contains NUL byte in the middle in hel null lo world\n");
  
  if (write(fd,data_with_nul,sizeof(data_with_nul))<0) // write data to device
    ERR("write() failed");
  
  // Read tokens
  char buf[128]; // buffer for token
  int token=0; // token counter
  int len; // length of read data
  int pass = 1; // flag to track test pass/fail

  // expected tokens
  const char expected1[] = {'h','e','l','\0','l','o'};
  int expected1_len = sizeof(expected1);
  const char expected2[] = "world";
  int expected2_len = strlen(expected2);
  // This loop reads tokens until no more tokens are available

  while (1) {
    len=read(fd,buf,sizeof(buf)); // read token from device
    if (len>0) { // we got token data
      printf("  Token %d: ",token);
      for (int i = 0; i < len; i++) {
        printf("%02x ", (unsigned char)buf[i]);
      }
      printf("(%d bytes)\n", len);
      // check expected tokens
      if (token == 0) {
        if (len != expected1_len || memcmp(buf, expected1, expected1_len) != 0) {
          pass = 0; // unexpected first token
        }
      } else if (token == 1) {
        if (len != expected2_len || memcmp(buf, expected2, expected2_len) != 0) {
          pass = 0; // unexpected second token
        }
      }
    } else if (len==0) { // end of token
      printf("   End of Token %d\n",token);
      token++;
    } else { // end of data (-1)
      printf("End of data\n");
      break;
      }
    }
    if (pass)
     printf("Test 5 result: PASS\n");
    else
      printf("Test 5 result: FAIL\n");
    close(fd);
  }

// Test 6: Empty write
void test6_empty_write() {
  printf("Test 6: Empty write\n");
  int fd=open("/dev/scanner",O_RDWR); // open device
  if (fd<0){
    ERR("open() failed");
  }
  int pass = 1; // flag to track test pass/fail

  if (write(fd,"",0)<0) // write empty data to device
    ERR("write() failed");

  // we call read inmediately
  int len = read(fd, NULL, 0); // attempt to read token
  if (len != -1) { // should return -1 for no data
    pass = 0; // unexpected read result
  }
  else {
    printf("Read returned -1 as expected for empty data\n");
  }
  if (pass)
    printf("Test 6 result: PASS\n");
  else
    printf("Test 6 result: FAIL\n");
  close(fd);
}

// Test 7: Multiple separators
void test7_multiple_separators() {
  printf("Test 7: Multiple Separators\n");
  int fd=open("/dev/scanner",O_RDWR); // open device
  if (fd<0){
    ERR("open() failed");
  }
  int pass = 1; // flag to track test pass/fail

  const char *data=":\t \n::  \t";  // all separators
  printf("  input: \"%s\" (only separators)\n", data);

  if (write(fd,data,strlen(data))<0) // write data to device
    ERR("write() failed");

  char buf[128]; // buffer for token
  int len; // length of read data
  int token =0; // token counter

  while (1) {
    len = read(fd, buf, sizeof(buf)); // read token from device
    if (len > 0) { // we got token data
      // Should NEVER get token data
      printf("  FAIL: Got unexpected token data \"%.*s\" (%d bytes)\n", len, buf, len);
      pass = 0;
    }
    else if (len == 0) { // end of token
      printf("   End of Empty token %d\n", token);
      token++;
    }
    else { // end of data (-1)
      printf("End of data\n");
      break;
    }
  }

  if (pass)
    printf("Test 7 result: PASS\n");
  else
    printf("Test 7 result: FAIL\n");
  close(fd);
}

void test8_multiple_instances() {
  printf("Test 8: Multiple Instances\n");
  
  int fd1 = open("/dev/scanner", O_RDWR);
  int fd2 = open("/dev/scanner", O_RDWR);
  
  if (fd1 < 0 || fd2 < 0) {
      ERR("Failed to open two instances");
  }
  int pass = 1; // flag to track test pass/fail
  
  // configure separators for fd1
  if (ioctl(fd1, 0, 0) < 0)
      ERR("ioctl() failed on fd1");

  const char *sep1 = "-,";
  if (write(fd1, sep1, strlen(sep1)) < 0)
      ERR("write() failed to set custom separators on fd1");
  
  // configure separators for fd2
  if (ioctl(fd2, 0, 0) < 0)
      ERR("ioctl() failed on fd2");

  const char *sep2 = ":";
  if (write(fd2, sep2, strlen(sep2)) < 0)
      ERR("write() failed to set custom separators on fd2");

  printf("fd1 separators: \"%s\"\n", sep1);
  printf("fd2 separators: \"%s\"\n", sep2);
  // write data to fd1 and fd2
  const char *data1 = "hello-world,miguel-carrasco";
  const char *data2 = "hola:mundo:hehe";

  if (write(fd1, data1, strlen(data1)) < 0)
      ERR("write() failed on fd1");
  if (write(fd2, data2, strlen(data2)) < 0)
      ERR("write() failed on fd2");
  
  // Read tokens
  char buf[128];
  int len;

  printf("Reading from fd1:\n");
  int token1 = 0;
  while (1) {
    len = read(fd1, buf, sizeof(buf));
    if (len > 0) {
      buf[len] = 0;
      printf("    fd1 token: \"%s\"\n", buf);
    } else if (len == 0) {
      token1++;
    } else {
      break;
    }
  }

  printf("Reading from fd2:\n");
  int token2 = 0;
  while (1) {
    len = read(fd2, buf, sizeof(buf));
    if (len > 0) {
      buf[len] = 0;
      printf("    fd2 token: \"%s\"\n", buf);
    } else if (len == 0) {
      token2++;
    } else {
      break;
    }
  }
  if (token1 != 4 || token2 != 3) {
      pass = 0; // unexpected number of tokens
       printf("  FAIL: token counts differ (fd1=%d, fd2=%d)\n", token1, token2);
  }
  if (pass)
    printf("Test 8 result: PASS\n");
  else
    printf("Test 8 result: FAIL\n");
  close(fd1);
  close(fd2);
}

// Test 9: Null separator
void test9_null_separator() {
  printf("Test 9: Null Separator\n");

  int fd=open("/dev/scanner",O_RDWR); // open device
  if (fd<0){
    ERR("open() failed");
  }

  // Switch to configuration mode
  if (ioctl(fd,0,0)<0) //
    ERR("ioctl() failed");
  
  // Set null separator
   char separators[2] = { '\0', ':' };
  if (write(fd,separators,sizeof(separators))<0)
    ERR("write() failed to set null separator");

  // Data containing embedded NULs
  char data[] = { 'h','e','\0','l','l','o','\0','w','o','r','l','d',
                  ':','t','h','i','s',':','i','s',':','a',':','t','e','s','t' };
   if (write(fd, data, sizeof(data)) < 0)
    ERR("write() failed");

  char buf[128];
  int len = 0;
  int token=0;
  int pass = 1; // flag to track test pass/fail

  const char *expected_tokens[] = { "he", "llo", "world", "this", "is", "a", "test" };
  while (1) {
    len=read(fd,buf,sizeof(buf)); // read token from device
    if (len>0) { // we got token data
      buf[len]=0; // null-terminate the string
      printf("  Token %d: \"%s\" (%d bytes)\n", token, buf, len);
      if (strcmp(buf, expected_tokens[token]) != 0) {
        pass = 0; // unexpected token
      }
    } else if (len==0) { // end of token
      token++;
    } else { // end of data (-1)
      break;
    }
  }
  if (token != 7) {
    pass = 0; // incorrect number of tokens
  }
  if (pass)
    printf("Test 9 result: PASS\n");
  else
    printf("Test 9 result: FAIL\n");
  close(fd);
}

// Test 10: No separators
void test10_no_separators() {
  printf("Test 10: No Separators\n");
  int fd=open("/dev/scanner",O_RDWR); // open device
  if (fd<0){
    ERR("open() failed");
  }
  // Switch to configuration mode
  if (ioctl(fd,0,0)<0) //
    ERR("ioctl() failed");
  
  // Set no separators
  if (write(fd,"",0)<0)
    ERR("write() failed to set no separators");
  
  const char*data = "test:no:separators";
  if (write(fd,data,strlen(data))<0) // write data to device
    ERR("write() failed");  
  
  char buf[128];
  int len = 0;
  int pass = 1; // flag to track test pass/fail
  int token =0;

  while (1) 
  {
    len=read(fd,buf,sizeof(buf)); // read token from device
    if (len>0) 
    { // we got token data
      buf[len]=0; // null-terminate the string
      printf("  Token %d: \"%s\"\n", token, buf);
      if (strcmp(buf, data) != 0) 
        pass = 0; // unexpected token
    } else if (len==0) 
    { // end of token
      token++;
    } else { // end of data (-1)
      break;
    }
  }
  if (token != 1) 
  {
    pass = 0; // incorrect number of tokens
  }
  
  if (pass)
    printf("Test 10 result: PASS\n");
  else
    printf("Test 10 result: FAIL\n");
  close(fd);
}

// Test 11: Stress test for memory leaks
void test11_stress_test() {
  printf("Test 11: Stress Test for Memory Leaks\n");
  int iterations = 500;
  int pass = 1; // flag to track test pass/fail
  
  // Repeat open, write, read, close multiple times
  for (int i = 0; i < iterations; i++) 
  {
    int fd=open("/dev/scanner",O_RDWR); // open device
    if (fd<0){
     printf("  FAIL: could not open device at iteration %d\n", i);
     pass = 0;
     break;
    }
    const char *data="leak:test:iteration";
    if (write(fd,data,strlen(data))<0){ // write datae to device
      printf("  FAIL: write() failed at iteration %d\n", i);
      pass = 0;
      close(fd);
      break;
    }

    char buf[128];
    int len;
    int token =0;

    while (1) {
      len=read(fd,buf,sizeof(buf)); // read token from device
      if (len>0) { // we got token data
        buf[len]=0; // null-terminate the string
      } else if (len==0) { // end of token
        token++;
      } else { // end of data (-1)
        break;
      }
    }
    close(fd);

    if (token != 3) {
      pass = 0; // incorrect number of tokens
    }
    if (i % 100 == 0) {
      printf("  Completed %d iterations\n", i);
    }
  }

  if (pass)
    printf("Test 11 result: PASS\n");
  else
    printf("Test 11 result: FAIL\n");
}
    


int main() {

  printf("=== Scanner Device Test ===\n");
  test1_default_separators();
  test2_custom_separators();
  test3_non_cumulative_writes();
  test4_partial_reads();
  test5_nul_bytes();
  test6_empty_write();
  test7_multiple_separators();
  test8_multiple_instances();
  test9_null_separator();
  test10_no_separators();
  test11_stress_test(); // for memmory leaks
  return 0;
}
