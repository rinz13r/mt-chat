#include <iostream>
#include <vector>
#include <unistd.h>
#include <signal.h>

std::vector<int> pids;

int spid;
void signal_handler (int signum) {
    for (auto & pid : pids) {
        kill (pid, SIGINT);
        std::cout << "\n" ;
    }
    sleep (1);
    kill(spid, SIGINT);
    std::cout << "\nend\n";
}

int main (int argc, char ** argv) {
    if (argc != 3 && argc != 4) {
        printf ("Usage: %s <serv_path> <#clients> <#pclients:optional>\n", argv[0]);
        exit (EXIT_FAILURE);
    }
    const char * port_str = "9013";
    spid = fork ();
    if (spid == 0) {
        execl (argv[1], argv[1], port_str, NULL);
    } else {
        usleep (100000);
        int n_clients = atoi (argv[2]), n_pclients = 0;
        if (argc == 4)
            n_pclients = atoi (argv[3]);
        else
            n_pclients = 1;
        for (int i = 0; i < n_pclients; ++i) {
            int pid = fork ();
            if (pid == 0) {
                execl ("bin/spam_client", "bin/spam_client", "0.0.0.0", port_str, "Vijay", "1", "0", NULL);
            } else {
                pids.push_back (pid);
            }
            usleep (20000);
        }
        for (int i = 0; i < n_clients; ++i) {
            int pid = fork ();
            if (pid == 0) {
                execl ("bin/test_client", "bin/test_client", "0.0.0.0", port_str, "Vijay", "1", "1", NULL);
            } else {
                pids.push_back (pid);
            }
            usleep (20000);
        }
        sleep (1);
        signal_handler (SIGINT);

    }

}
