module.exports = {
  title: "pesy",
  tagline: "Assisting your npm workflow for native Reason/OCaml",
  url: "https://pesy.github.io",
  baseUrl: "/",
  favicon: "img/favicon.ico",
  organizationName: "esy",
  projectName: "pesy",
  themeConfig: {
    navbar: {
      logo: {
        alt: "pesy",
        src: "img/logo.svg"
      },
      links: [
        { to: "docs/introduction", label: "Documentation", position: "right" },
        { to: "blog", label: "Blog", position: "right" },
        {
          href: "https://github.com/esy/pesy",
          label: "GitHub",
          position: "right"
        }
      ]
    },
    footer: {
      style: "dark",
      links: [
        {
          title: "Documentation",
          items: [
            {
              label: "Getting Started",
              to: "docs/introduction"
            },
            {
              label: "Reference",
              to: "docs/supported-config"
            }
          ]
        },
        {
          title: "Community",
          items: [
            {
              label: "Stack Overflow",
              href: "https://stackoverflow.com/questions/tagged/pesy"
            },
            {
              label: "Discord",
              href: "https://discord.gg/xJHsgY6"
            }
          ]
        },
        {
          title: "Social",
          items: [
            {
              label: "Blog",
              to: "blog"
            },
            {
              label: "GitHub",
              href: "https://github.com/esy/pesy"
            }
          ]
        }
      ],
      copyright: `Copyright © ${new Date().getFullYear()} pesy with Reason`
    }
  },
  presets: [
    [
      "@docusaurus/preset-classic",
      {
        docs: {
          sidebarPath: require.resolve("./sidebars.js"),
          editUrl: "https://github.com/esy/pesy/edit/master/site/"
        },
        theme: {
          customCss: require.resolve("./src/css/custom.css")
        }
      }
    ]
  ]
};
