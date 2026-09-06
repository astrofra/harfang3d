import argparse
import os
from pathlib import Path
import zipfile


def zip_directory(source: Path, destination: Path) -> None:
    temporary = destination.with_suffix(destination.suffix + ".tmp")

    try:
        with zipfile.ZipFile(
            temporary,
            "w",
            compression=zipfile.ZIP_DEFLATED,
            allowZip64=True,
        ) as archive:
            archive.write(source, f"{source.name}/")
            for path in sorted(
                source.rglob("*"),
                key=lambda item: item.relative_to(source).as_posix().lower(),
            ):
                archive_name = Path(source.name) / path.relative_to(source)
                archive.write(path, archive_name.as_posix())

        os.replace(temporary, destination)
    except BaseException:
        temporary.unlink(missing_ok=True)
        raise


def main() -> int:
    parser = argparse.ArgumentParser(
        description="Zippe chaque sous-dossier d'installation dans une archive Windows 64 bits."
    )
    parser.add_argument("install_dir", type=Path)
    args = parser.parse_args()

    install_dir = args.install_dir.resolve()
    if not install_dir.is_dir():
        parser.error(f"dossier d'installation introuvable: {install_dir}")

    directories = sorted(
        (path for path in install_dir.iterdir() if path.is_dir()),
        key=lambda path: path.name.lower(),
    )
    if not directories:
        print(f"Aucun dossier a zipper dans: {install_dir}")
        return 0

    for directory in directories:
        archive = install_dir / f"hg-{directory.name}-win64.zip"
        print(f"Creation: {archive}")
        zip_directory(directory, archive)

    print(f"\n{len(directories)} archive(s) creee(s) dans: {install_dir}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
