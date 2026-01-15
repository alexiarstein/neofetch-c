inc_src = include_directories('../src', '../include')

src_files = files([
    'ascii.c',
    'sysinfo.c',
    'utils.c',
    'neofetch.c'
])


install_subdir('../ascii/', install_dir: get_option('prefix') / 'share/neofetch/') #copy all ascci dir

executable('neofetch',
    sources: src_files,
    include_directories: [inc_src],
    install: true
)

