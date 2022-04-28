# === subdirectories ==============================================

add_subdirectory( rdsc )

# === this directory ==============================================

add_rn_library(
  rn-rds-util
  # Dependencies
  rn-base
  rn-cdr
  rn-refl
)
