{
  description = "DevShell for ns-3 analysis environment.";

  inputs.nixpkgs.url = "github:nixos/nixpkgs/nixos-25.05";

  outputs = {
    self,
    nixpkgs,
  }: let
    pkgs = nixpkgs.legacyPackages.x86_64-linux;
  in {
    devShells.x86_64-linux.default = pkgs.mkShell {
      packages = with nixpkgs.legacyPackages.x86_64-linux; [
        # Prerequirements
        git
        python312

        # optional
        tcpdump
        wireshark
        doxygen # Documentation generation tool
        dia # Diagramming tool
        texliveFull # LaTeX document preparation system
        python312Packages.sphinx # Documentation generation tool
        graphviz # Graph visualization software
        kdePackages.qtbase # KDE base packages for netanim
        kdePackages.qttools # Qt tools for netanim

        # ns-3 analysis tools
        python312Packages.ipykernel
        python312Packages.jupyter
        python312Packages.jupyterlab
        python312Packages.notebook
        python312Packages.matplotlib # Plotting library for Python
        python312Packages.plotly # Interactive graphing library for Python
        python312Packages.ipywidgets
        python312Packages.pandas # Data analysis and manipulation library for Python
        python312Packages.seaborn
      ];

      shellHook = ''
        echo "📶 ns-3 analysis environment ready!"
      '';
    };
  };
}
