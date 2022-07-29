# wasmWabbitemu

## What is wasmWabbitemu?

wasmWabbitemu is port of the wxWabbitemu a TI-8x emulator to the web.
It is based on a decent TI-8x emulator, Wabbitemu.

## Online demo

http://marcuswatkins.net/wasmWabbitemu/

## Building and running

Building currently requires docker. After docker is installed simply run `devhtml.sh` to build and start a simple webserver:

```bash
./devhtml.sh
```

You can set optional environment variables for `PORT`, `CONTAINER_NAME` and `IMAGE_NAME` if needed.

Once started visit http://127.0.0.1:8081/ in a browser and load a ROM.

## Extracting static content

If you'd prefer to run under a pre-existing webserver copy the `/public` folder out of the docker image.

```bash
docker cp wabbit /public /my/webserver/htdocs/
```

## Status

Currently the only skin is a TI-85, but this can be easily extended by editing the CSS (keymaps and colors are pure CSS).

## License

See [LICENSE](License)

[albert]: https://github.com/alberthdev
[buckeye]: https://github.com/BuckeyeDude
[dgomes]: https://github.com/davidgomes
[jonimus]: https://github.com/Jonimoose
[geekboy]: https://github.com/geekbozu
[marwatk]: https://github.com/marwatk