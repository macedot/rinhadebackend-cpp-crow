CREATE OR REPLACE FUNCTION generate_searchable(_nome VARCHAR, _apelido VARCHAR, _stack VARCHAR)
RETURNS TEXT AS $$
    BEGIN
        RETURN _nome || _apelido || _stack;
    END;
$$ LANGUAGE plpgsql IMMUTABLE;

CREATE TABLE IF NOT EXISTS public.pessoas (
    id          UUID DEFAULT gen_random_uuid() UNIQUE NOT NULL,
    apelido     TEXT UNIQUE NOT NULL,
    nome        TEXT NOT NULL,
    nascimento  DATE NOT NULL,
    stack       TEXT,
    searchable  TEXT GENERATED ALWAYS AS (generate_searchable(nome, apelido, COALESCE(stack, ''))) STORED
);

CREATE UNIQUE INDEX IF NOT EXISTS pessoas_apelido_index
    ON public.pessoas
    USING btree (apelido);