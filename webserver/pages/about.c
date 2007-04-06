void generate_about(int sock_in, char *query, char *ip)
{
//printf("%s", html_header("About"));
	web_send(sock_in, html_header("About"));
	web_send(sock_in, "<center>Freya Web Server!</center>\n");
}
