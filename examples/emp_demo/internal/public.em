// Exported const: should be visible via `use {..} from ...`.
export const BUILD_NUMBER: i32 = 1;

// Not exported: should NOT be visible via normal imports.
const INTERNAL_ONLY_FLAG: i32 = 0;
