{ nixpkgs ? <nixpkgs>
, systems ? [ "i686-linux" "x86_64-linux" ]
, libprocreact ? { outPath = ./.; rev = 1234; }
, officialRelease ? false
}:

let
  pkgs = import nixpkgs {};

  jobs = rec {
    tarball =
      with pkgs;

      releaseTools.sourceTarball {
        name = "libprocreact-tarball";
        version = builtins.readFile ./version;
        src = libprocreact;
        inherit officialRelease;
        CFLAGS = "-Wall -std=gnu90";
        buildInputs = [ doxygen ];

        preDist = ''
          make -C src apidox
          mkdir -p $out/share/doc/libprocreact
          cp -av apidox $out/share/doc/libprocreact
          echo "doc api $out/share/doc/libprocreact/apidox/html" >> $out/nix-support/hydra-build-products
        '';
      };

    build =
      pkgs.lib.genAttrs systems (system:
        with import nixpkgs { inherit system; };

        releaseTools.nixBuild {
          name = "libprocreact";
          src = tarball;
          CFLAGS = "-Wall -std=gnu90";
          doCheck = true;
        });
  };
in
jobs
