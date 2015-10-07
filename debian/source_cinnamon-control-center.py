import os, apport.packaging, re
from apport.hookutils import *

def add_info(report):
    # the issue is not in the cinnamon-control-center code so reassign
    if "Stacktrace" in report and "/usr/lib/cinnamon-control-center-1" in report["Stacktrace"]:
        for words in report["Stacktrace"].split():
            if words.startswith("/usr/lib/cinnamon-control-center-1"):
                if apport.packaging.get_file_package(words) != 'cinnamon-control-center':
                    report.add_package_info(apport.packaging.get_file_package(words))
                    return
                component = re.compile("lib(\w*).so").search(words).groups(1)[0]
                report['Title'] = '[%s]: %s' % (component, report.get('Title', report.standard_title()))
                report['Tags'] = '%s %s' % (report.get('Tags', ""), component)
                break # Stop on the first .so that's the interesting one

    # collect informations on the /usr/lib/control-center-1 components
    plugin_packages = set()
    for dirpath, dirnames, filenames in os.walk("/usr/lib/cinnamon-control-center-1"):
        for filename in filenames:
            path = os.path.join(dirpath, filename)
            package = apport.packaging.get_file_package(path)
            if package == 'cinnamon-control-center':
                continue

            plugin_packages.add(package)

    report["usr_lib_cinnamon-control-center"] = package_versions(*sorted(plugin_packages))
