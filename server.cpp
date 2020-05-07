//Step 1
// Server side C/C++ program to demonstrate Socket programming
//Source: https://www.geeksforgeeks.org/socket-programming-cc/

//Step 2
//Klasse ClearingServerSocket

//Step 3
//Threading
//https://dzone.com/articles/parallel-tcpip-socket-server-with-multi-threading
//https://gist.github.com/oleksiiBobko/43d33b3c25c03bcc9b2b

//Step 4
//Das mit der Klasse ist eine Fehlentwicklung, das macht es in dem kleinen Beispiel nur
//komplex, wieder raus, und dann schauen wir mal.

//Step 5
//SSL
// ein einfaches Beispiel was nicht tut, vermutlich OpenSSL 1.0.2 vs 1.1.0
//   https://wiki.openssl.org/index.php/Simple_TLS_Server
// etwa unverständlich
//   https://quuxplusone.github.io/blog/2020/01/24/openssl-part-1/
// Sourcen aus github geladen, da sind demos drin.
//   https://github.com/openssl/openssl
// Über die Erzeugung von Schlüsseln
//   https://www.grund-wissen.de/linux/server/openssl.html
// Hier wird das Zertifikat abgefragt
//  http://fm4dd.com/openssl/sslconnect.shtm
//ERKENNTNIS
//  eigentlich fehlte im Client nur SSL_CTX_new(SSLv23_client_method()); statt SSL_CTX_new(TLS_server_method());

//Step 6
//Authorisierung, mutal TLS (mTSL)
//something is wrong with the certificates

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
    char cert[MAXFILENAME+1] = "/home/debdev/Projects/clearing/server/certs/server/server.crt";
    char key[MAXFILENAME+1] = "/home/debdev/Projects/clearing/server/certs/server/server.key";
    int serverSocket = -1;
};

class connectionContext
{
  public:
    int socket = -1;
    serverContext *sCtx = NULL;
};

SSL_CTX *create_context()
{
    SSL_CTX *ctx;

    ctx = SSL_CTX_new(TLS_server_method());
    if (!ctx)
      {
	      perror("Unable to create SSL context");
	      ERR_print_errors_fp(stderr);
	      exit(EXIT_FAILURE);
      }

    return ctx;
}

void configure_context(serverContext *srvCtx)
{
    //int verify_flags = SSL_VERIFY_PEER or SSL_VERIFY_FAIL_IF_NO_PEER_CERT  //meine eigene Lösung
    int verify_flags = SSL_VERIFY_PEER | SSL_VERIFY_CLIENT_ONCE | SSL_VERIFY_FAIL_IF_NO_PEER_CERT;
    //int verify_flags =SSL_VERIFY_CLIENT_ONCE; //Viktor 006.05.2020
    //int verify_flags = SSL_VERIFY_PEER; //Viktor 006.05.2020

    SSL_CTX_set_verify(srvCtx->ctx, verify_flags, NULL);
    SSL_CTX_set_verify_depth(srvCtx->ctx, 5);

    char cafile[] = "/home/debdev/Projects/clearing/server/certs/server/ca.crt";

    if (!SSL_CTX_use_certificate_chain_file(srvCtx->ctx, cafile))
      {
        ERR_print_errors_fp(stderr);
        exit(EXIT_FAILURE);
      }
    SSL_CTX_load_verify_locations(srvCtx->ctx, cafile, NULL);

    /*
    STACK_OF(X509_NAME) *calist = SSL_load_client_CA_file(cafile);
    if (calist == 0) {
        /* Not generally critical
        printf("error loading client CA names from: %s",cafile);
        //msg_warn("error loading client CA names from: %s",cafile);
        //tls_print_errors();
    }
    SSL_CTX_set_client_CA_list(srvCtx->ctx, calist);
    */

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
      * Versuch das Client-Zertifikat zu lesen
      */
     X509 *cert = SSL_get_peer_certificate(ssl);
     if (cert == NULL)
       printf("Error: Could not get a certificate from: %s.\n", "");
     else
       printf("Retrieved the server's certificate from: %s.\n", "");

     X509_NAME *certname = X509_NAME_new();
     certname = X509_get_subject_name(cert);

     printf("Displaying the certificate subject data:\n");
     BIO *  bio  = BIO_new_fp(stdout, BIO_NOCLOSE);
     X509_NAME_print_ex(bio, certname, 0, 0);
     printf("\n");

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
      else if ((strcmp(argv[i],"-cert")==0) && (i+1 < argc) && (strlen(argv[i+1]) > 0))
        strncpy(srvCtx->cert,argv[i+1],MAXFILENAME);
      else if ((strcmp(argv[i],"-key")==0) && (i+1 < argc) && (strlen(argv[i+1]) > 0))
        strncpy(srvCtx->key,argv[i+1],MAXFILENAME);
    }

  //argument -plain cca deaktivate ssl for test purposes (it is all an example)
  if (srvCtx->withSSL = true)
    {
      srvCtx->ctx = create_context();
      configure_context(srvCtx);
      printf("mit SSL\n");
    }

  socket_setup(&address,srvCtx);
  socket_loop(&address,srvCtx);

  return 0;
}
