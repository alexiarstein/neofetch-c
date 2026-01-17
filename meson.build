project('neofetch', 
'c',
 version: '0.1.0',
 default_options : ['c_std=c11','warning_level=3', 'default_library=static']
 )

add_project_arguments(
  '-fstack-protector-strong',
  '-fPIE',
  '-Wformat',
  '-Werror=format-security',
  '-D_GNU_SOURCE',
  '-D_FORTIFY_SOURCE=2 ',
  language: 'c'
)
 subdir('src')
