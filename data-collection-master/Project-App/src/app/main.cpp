
extern int app_main(int argc, char **argv, bool zloginit);
int main(int argc, char **argv)
{
    return app_main(argc, argv, true);
}