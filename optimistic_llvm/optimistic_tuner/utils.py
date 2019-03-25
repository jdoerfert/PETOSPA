import json


def is_valid_input(i, re):
    return True


def get_valid_input(re):
    inp = input('>> ')
    if not is_valid_input(inp, re):
        return get_valid_input(re)
    return inp


def select_from_list(lst):
    for i, item in enumerate(lst):
        print(f'\t{i}) {item}')
    return get_valid_input('')


class serializable(object):
    classes = {}

    @staticmethod
    def class_encoder(val):
        if '__type__' in dir(type(val)):
            dct = {'__type__': val.__type__}
            # dct = {}
            for k, v in val.__dict__.items():
                if not k.startswith('_'):
                    dct[k] = serializable.class_encoder(v)
            return dct
        return val

    @staticmethod
    def class_decoder(dct):
        if '__type__' in dct:
            type = dct['__type__']
            del dct['__type__']
            return serializable.classes[type](**dct)
        return dct

    @staticmethod
    def from_json(json_str):
        return json.loads(json_str, object_hook=serializable.class_decoder)

    def to_json(self):
        return json.dumps(self, default=serializable.class_encoder, indent=4)
