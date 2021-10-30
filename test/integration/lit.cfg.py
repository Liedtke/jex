import lit.formats

config.name = 'jexc'
config.test_format = lit.formats.ShTest(True)

config.suffixes = ['.jex']

config.test_source_root = os.path.dirname(__file__)
config.test_exec_root = config.my_integration_bld

config.substitutions.append(('%jexc',
    os.path.join(config.my_bld_dir, 'tools/jexc')))
