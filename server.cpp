/**
 * Proof of concept for mutual TLS
 */

//Step 1
// Server side C/C++ program to demonstrate Socket programming
//Source: https://www.geeksforgeeks.org/socket-programming-cc/

//Step 2
//class ClearingServerSocket

//Step 3
//Threading
//https://dzone.com/articles/parallel-tcpip-socket-server-with-multi-threading
//https://gist.github.com/oleksiiBobko/43d33b3c25c03bcc9b2b

//Step 4
//class ClearingServerSocket, was the wrong way, removed

//Step 5
//SSL
// easy example not working, I believe OpenSSL 1.0.2 vs 1.1.0
//   https://wiki.openssl.org/index.php/Simple_TLS_Server
// not easy to understand
//   https://quuxplusone.github.io/blog/2020/01/24/openssl-part-1/
// load openssl sources, there are examples and demos in
//   https://github.com/openssl/openssl
// creating keys and certificates
//   https://www.grund-wissen.de/linux/server/openssl.html
// how to get a certificate
//  http://fm4dd.com/openssl/sslconnect.shtm
//Lesson learned
//  the clients needs SSL_CTX_new(SSLv23_client_method()) instead of SSL_CTX_new(TLS_server_method())

//Step 6
//  Authorisierung, mutal TLS (mTSL)
//  https://github.com/TalkWithTLS

//Step 7
//  reading some information from the client zertificate (CN und Valid-Time)

//  ToDo

//Step 8
//  JSON-Handling, that wie can interact
//    https://github.com/DaveGamble/cJSON
//    https://github.com/json-c/json-c

//Step 9
//  creating,renewal certificate from client via server
//  https://stackoverflow.com/questions/256405/programmatically-create-x509-certificate-using-openssl

#include <unistd.h>
#include <stdio.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <string.h>
#include <pthread.h>
#include <openssl/ssl.h>
#include <openssl/x509v3.h>
#include <openssl/err.h>

#define PORT 8080
#define MAXFILENAME 127

class serverContext
{
  public:
    bool withSSL = true;
    SSL_CTX *ctx = NULL;
    char cafile[MAXFILENAME+1] = "../certs/server/ca.crt";
    char cert[MAXFILENAME+1] = "../certs/server/server.crt";
    char key[MAXFILENAME+1] = "../certs/server/server.key";
    int serverSocket = -1;
};

class connectionContext
{
  public:
    int socket = -1;
    serverContext *sCtx = NULL;
};

 void create_ssl_context(serverContext *srvCtx)
{
  srvCtx->ctx = SSL_CTX_new(TLS_server_method());
  if (!srvCtx->ctx)
    {
      perror("Unable to create SSL context");
      ERR_print_errors_fp(stderr);
      exit(EXIT_FAILURE);
    }

  int verify_flags = SSL_VERIFY_PEER | SSL_VERIFY_CLIENT_ONCE | SSL_VERIFY_FAIL_IF_NO_PEER_CERT;

  SSL_CTX_set_verify(srvCtx->ctx, verify_flags, NULL);
  SSL_CTX_set_verify_depth(srvCtx->ctx, 5);

  if (!SSL_CTX_use_certificate_chain_file(srvCtx->ctx, srvCtx->cafile))
    {
      ERR_print_errors_fp(stderr);
      exit(EXIT_FAILURE);
    }
  SSL_CTX_load_verify_locations(srvCtx->ctx, srvCtx->cafile, NULL);

  /* Set the key and cert */
  if (SSL_CTX_use_certificate_file(srvCtx->ctx, srvCtx->cert, SSL_FILETYPE_PEM) <= 0)
    {
      ERR_print_errors_fp(stderr);
      exit(EXIT_FAILURE);
    }
  if (SSL_CTX_use_PrivateKey_file(srvCtx->ctx, srvCtx->key, SSL_FILETYPE_PEM) <= 0 )
    {
      ERR_print_errors_fp(stderr);
      exit(EXIT_FAILURE);
    }

  /* verify private key */
  if (!SSL_CTX_check_private_key(srvCtx->ctx) )
    {
      printf("Private key does not match the public certificate\n");
      ERR_print_errors_fp(stderr);
      abort();
    }
}

void socket_setup(sockaddr_in *address, serverContext *srvCtx)
{
	int opt = 1;

	// Creating socket file descriptor
	if ((srvCtx->serverSocket = socket(AF_INET, SOCK_STREAM, 0)) == 0)
	{
		perror("socket failed");
		exit(EXIT_FAILURE);
	}

	// Forcefully attaching socket to the port 8080
	if (setsockopt(srvCtx->serverSocket, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT,
												&opt, sizeof(opt)))
	{
		perror("setsockopt");
		exit(EXIT_FAILURE);
	}

	// Forcefully attaching socket to the port 8080
	if (bind(srvCtx->serverSocket, (struct sockaddr *)address, sizeof(*address))<0)
	{
		perror("bind failed");
		exit(EXIT_FAILURE);
	}
	if (listen(srvCtx->serverSocket, 3) < 0)
	{
		perror("listen");
		exit(EXIT_FAILURE);
	}
}

void *connection_handler(void *socket_desc)
{
  //Get the socket descriptor
   connectionContext *cc = (connectionContext*)socket_desc;

   printf("Beginn connection handling.\n");

   char buffer[1024];
   char hello[] = "Hello from server via SSL";

   SSL *ssl = NULL;
   if (cc->sCtx->withSSL)
    {
       ssl = SSL_new(cc->sCtx->ctx);
       if (ssl == NULL)
         ERR_print_errors_fp(stderr);

       if (!SSL_set_fd(ssl, cc->socket))
         ERR_print_errors_fp(stderr);

       if (SSL_accept(ssl) <= 0)
         {
           ERR_print_errors_fp(stderr);
         }
     }

   /**
    * Client-Zertifikat lesen und anzeigen
    */
   X509 *cert = SSL_get_peer_certificate(ssl);
   if (cert == NULL)
     printf("Error: Could not get a certificate from: %s.\n", "");
   else
     printf("Retrieved the server's certificate from: %s.\n", "");

   X509_NAME *certname = X509_NAME_new();
   certname = X509_get_subject_name(cert);

   printf("Displaying the certificate subject data:\n");
   BIO *bio  = BIO_new_fp(stdout, BIO_NOCLOSE);
   X509_NAME_print_ex(bio, certname, 0, 0);
   printf("\n");

   /**
    * Extract the CN field
    */
   int cnidx = X509_NAME_get_index_by_NID(X509_get_subject_name(cert), NID_commonName, -1);
	 //if (common_name_loc < 0) { }
   X509_NAME_ENTRY *cnentry = X509_NAME_get_entry(X509_get_subject_name((X509 *) cert), cnidx);
   //if (common_name_entry == NULL) { }
   // Convert the CN field to a C string
   ASN1_STRING *cnasn1 = X509_NAME_ENTRY_get_data(cnentry);
   //if (common_name_asn1 == NULL) { }
   char *cn = (char *)ASN1_STRING_get0_data(cnasn1);
   printf("CN: %s\n",cn);

   /**
    * Extract validity dates
    */
   const ASN1_TIME *afterASN1 = X509_get0_notAfter(cert);
   char *after = (char *)ASN1_STRING_get0_data(afterASN1);  //210506093551Z  yymmddhhnnssZ
   printf("AFTER: %s\n",after);

   if (cc->sCtx->withSSL)
     SSL_read( ssl, buffer, 1024);
   else
     read(cc->socket , buffer, 1024);
   printf("out: %s\n",buffer);

   if (cc->sCtx->withSSL)
      SSL_write(ssl, hello, strlen(hello)+1); //+1, damit das \0 mitgesendet wird.
   else
      send(cc->socket , hello , strlen(hello)+1 , 0);
 	 printf("Hello server message sent\n");

   if (cc->sCtx->withSSL)
     {
       SSL_shutdown(ssl);
       SSL_free(ssl);
     }

   close(cc->socket);

   pthread_exit(NULL);
}

void socket_loop(sockaddr_in *address, serverContext *srvCtx)
{
  int valread = 0;
  char buffer[1024] = {0};
  int client_socket = 0;
  int addrlen = sizeof(address);
  pthread_t thread_id;

  printf("Waiting for socket!\n");
  while( client_socket = accept(srvCtx->serverSocket, (struct sockaddr *)&address, (socklen_t*)&addrlen) )
      {
          printf("Connection accepted\n");

          connectionContext* cc = new connectionContext();

          cc->sCtx = srvCtx;
          cc->socket = client_socket;
          if( pthread_create(&thread_id , NULL ,  connection_handler, (void*)cc) < 0)
            {
              printf("could not create thread\n");
            }
      }

  printf("End of socket_loop\n");
  pthread_exit(NULL);
}

int main(int argc, char const *argv[])
{
  struct sockaddr_in address;
  address.sin_family = AF_INET;
	address.sin_addr.s_addr = INADDR_ANY;
	address.sin_port = htons( PORT );

  serverContext *srvCtx = new serverContext();

  //some overhead for the arguments
  for (int i=0; i<argc; ++i)
    {
      if (strcmp(argv[i],"-plain")==0)
        srvCtx->withSSL = false;
      else if ((strcmp(argv[i],"-cafile")==0) && (i+1 < argc) && (strlen(argv[i+1]) > 0))
        strncpy(srvCtx->cafile,argv[i+1],MAXFILENAME);
      else if ((strcmp(argv[i],"-cert")==0) && (i+1 < argc) && (strlen(argv[i+1]) > 0))
        strncpy(srvCtx->cert,argv[i+1],MAXFILENAME);
      else if ((strcmp(argv[i],"-key")==0) && (i+1 < argc) && (strlen(argv[i+1]) > 0))
        strncpy(srvCtx->key,argv[i+1],MAXFILENAME);
    }

  //argument -plain cca deaktivate ssl for test purposes (it is all an example)
  if (srvCtx->withSSL = true)
    {
      create_ssl_context(srvCtx);
      printf("mit SSL\n");
    }

  socket_setup(&address,srvCtx);
  socket_loop(&address,srvCtx);

  return 0;
}
