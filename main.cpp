#include "include/libtorrent/torrent_info.hpp"

int main(int argc, char const* argv[])
{
        if (argc != 2) {
                fprintf(stderr, "usage: %s <magnet-url>\n");
                return 1;
        }
        lt::session ses;

        lt::add_torrent_params atp = lt::parse_magnet_uri(argv[1]);
        atp.save_path = "."; // save in current dir
        lt::torrent_handle h = ses.add_torrent(atp);

        // ...

	return 1;
}
