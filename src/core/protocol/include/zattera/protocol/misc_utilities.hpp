#pragma once

namespace zattera { namespace protocol {

// TODO:  Rename these curves to match naming in manual.md
enum curve_id
{
   quadratic,
   quadratic_curation,
   linear,
   square_root
};

} } // zattera::utilities


FC_REFLECT_ENUM(
   zattera::protocol::curve_id,
   (quadratic)
   (quadratic_curation)
   (linear)
   (square_root)
)
