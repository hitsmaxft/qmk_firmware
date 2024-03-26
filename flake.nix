{
  description = "qmk firmware";

  inputs = {
      flake-utils.url = "github:numtide/flake-utils";
      annepro2-tools = {
          url = "github:hitsmaxft/nix-annepro2-tools/master";
          inputs = {
              flake-utils.follows = "flake-utils";
          };
      };
  };

  outputs = { self, nixpkgs, flake-utils, annepro2-tools }:
    flake-utils.lib.eachDefaultSystem
      (system:
      let

          pkgs = nixpkgs.legacyPackages.${system};
          shell = import ./shell.nix { inherit pkgs; };
      in
        {
            devShells.default =  shell.overrideAttrs (old: {
                nativeBuildInputs = old.nativeBuildInputs ++ [
                    annepro2-tools.defaultPackage.${system}
                ];
            });
        }
      );
}
