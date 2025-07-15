import os
import shutil
from PIL import Image
import argparse


def copy_files_with_duplicates(source_folder, target_folder, num_copies, convert_to_jpeg_cmyk=False):
    # Ensure source folder exists
    if not os.path.isdir(source_folder):
        raise ValueError(f"Source folder '{source_folder}' does not exist.")

    # Create target folder if it doesn't exist
    os.makedirs(target_folder, exist_ok=True)

    # Iterate through each item in the source folder
    for filename in os.listdir(source_folder):
        source_file_path = os.path.join(source_folder, filename)

        if os.path.isfile(source_file_path):
            name, ext = os.path.splitext(filename)

            is_image = ext.lower() in ['.png', '.jpg', '.jpeg', '.tiff', '.bmp', '.gif']

            if convert_to_jpeg_cmyk and is_image:
                try:
                    with Image.open(source_file_path) as img:
                        cmyk_img = img.convert('CMYK')
                        for i in range(1, num_copies + 1):
                            new_filename = f"{name}_copy{i}.jpg"
                            target_file_path = os.path.join(target_folder, new_filename)
                            cmyk_img.save(target_file_path, 'JPEG')
                            print(f"Converted and copied: {filename} -> {new_filename}")
                except Exception as e:
                    print(f"Failed to convert {filename}: {e}. Copying original file instead.")
                    # Fallback to original copy logic
                    for i in range(1, num_copies + 1):
                        new_filename = f"{name}_copy{i}{ext}"
                        target_file_path = os.path.join(target_folder, new_filename)
                        shutil.copy2(source_file_path, target_file_path)
                        print(f"Copied: {filename} -> {new_filename}")
            else:
                for i in range(1, num_copies + 1):
                    new_filename = f"{name}_copy{i}{ext}"
                    target_file_path = os.path.join(target_folder, new_filename)

                    shutil.copy2(source_file_path, target_file_path)
                    print(f"Copied: {filename} -> {new_filename}")


if __name__ == "__main__":
    parser = argparse.ArgumentParser(
        description="Copy files and create a specified number of duplicates. Optionally, convert image files to JPEG CMYK."
    )
    parser.add_argument("source", help="The source folder containing files to copy.")
    parser.add_argument("target", help="The target folder where copies will be saved.")
    parser.add_argument(
        "-n",
        "--num_copies",
        type=int,
        default=1,
        help="The number of copies to create for each file (default: 1).",
    )
    parser.add_argument(
        "-c",
        "--convert",
        action="store_true",
        help="Convert image files to JPEG CMYK instead of copying them directly.",
    )

    args = parser.parse_args()

    copy_files_with_duplicates(
        args.source,
        args.target,
        args.num_copies,
        convert_to_jpeg_cmyk=args.convert
    )
