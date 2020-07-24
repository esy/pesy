(function($) {
  var $window = $(window);
  var $document = $(document);

 /*
  * Scrollspy.
  */

 $document.on('flatdoc:ready', function() {
    $("h2, h3").scrollagent({parent: $(document.body)}, function(cid, pid, currentElement, previousElement) {
      if (pid) {
       $("[href='#"+pid+"']").removeClass('active');
      }
      if (cid) {
       $("[href='#"+cid+"']").addClass('active');
      }
    });
  });

  /*
   * Anchor jump links.
   */
  $document.on('flatdoc:ready', function() {
    $('.menu a').anchorjump();
  });

  /*
   * Title card.
   */
  $document.on('flatdoc:style-ready', function() {
     var $card = $('.title-card');
     if (!$card.length) return;

     var $header = $('.header');
     var headerHeight = $header.length ? $header.outerHeight() : 0;

     $window
       .on('resize.title-card', function() {
         var windowWidth = $window.width();

         if (windowWidth < 480) {
           $card.css('height', '');
         } else {
           var height = $window.height();
           $card.css('height', height - headerHeight);
         }
       })
       .trigger('resize.title-card');
  });

   
  /**
   * Remove any nodes that are not needed once rendered. This way when generating
   * a pre-rendered `.rendered.html`, they won't become part of the bundle, when
   * that rendered page is turned into a `.html` bundle. They have served their
   * purpose. Add `class='removeFromRenderedPage'` to anything you want removed
   * once used to render the page. (Don't use for script tags that are needed for
   * interactivity).
   */
  $document.on('flatdoc:ready', function() {
    $('.removeFromRenderedPage').each(function(i, e) {
      e.parentNode.removeChild(e);
    });
  });
   
  /**
   * If you add a style="visibility:hidden" to your document body, we will clear
   * the style after the styles have been injected. This avoids a flash of
   * unstyled content.
   */
  $document.on('flatdoc:ready', function() {
    document.body.style="";
  });

  /**
   * If you add a style="visibility:hidden" to your document body, we will clear
   * the style after the styles have been injected. This avoids a flash of
   * unstyled content.
   */
  $document.on('flatdoc:ready', function() {
    document.body.style="";
  });

   
  /**
   * See documentation for `continueRight` css class in style.styl.
   */
  $document.on('flatdoc:ready', function() {
     document.querySelectorAll(
        '.content > p + pre,' +
        '.content > p + blockquote,' +
        '.content > ul + pre,' +
        '.content > ul + blockquote,' +
        '.content > ol + pre,' +
        '.content > ol + blockquote,' +
        '.content > h1 + pre,' +
        '.content > h1 + blockquote,' +
        '.content > h2 + pre,' +
        '.content > h2 + blockquote,' +
        '.content > h3 + pre,' +
        '.content > h3 + blockquote,' +
        '.content > h4 + pre,' +
        '.content > h4 + blockquote,' +
        '.content > h5 + pre,' +
        '.content > h5 + blockquote,' +
        '.content > h6 + pre,' +
        '.content > h6 + blockquote,' +
        '.content > table + pre,' +
        '.content > table + blockquote'
      ).forEach(function(e) {
        // Annotate classes for the left and right items that are "resynced".
        // This allows styling them differently. Maybe more top margins.
        e.className += 'flatdoc-synced-up-right';
        if(e.previousSibling) {
           e.previousSibling.className += 'flatdoc-synced-up-left';
        }
      })
  });
  
  $(document).on('flatdoc:ready', function() {
    $("#misc, #basic").remove();

    $("pre > code").each(function() {
      var $code = $(this);
      var m = $code.text().match(/<body class='([^']*)'/);
      if (m) {
        var $q = $("<blockquote><a href='#"+m[1]+"' class='button light'>Toggle</a></blockquote>");
        $q.find('a').click(function() {
          var klass = $(this).attr('href').substr(1);
          $('body').toggleClass(klass);
          if (klass === 'big-h3') $.anchorjump('#theme-options');
          if (klass === 'large-brief') $.anchorjump('#flatdoc');

        });
        $code.after($q);
      }
    });
  });
  

  /*
   * Sidebar stick.
   */

  /*
  $(function() {
    var $sidebar = $('.menubar');
    var elTop;

    $window
      .on('resize.sidestick', function() {
        $sidebar.removeClass('fixed');
        elTop = $sidebar.offset().top;
        $window.trigger('scroll.sidestick');
      })
      .on('scroll.sidestick', function() {
        var scrollY = $window.scrollTop();
        $sidebar.toggleClass('fixed', (scrollY >= elTop));
      })
      .trigger('resize.sidestick');
  });
  */

})(jQuery);
/*! jQuery.scrollagent (c) 2012, Rico Sta. Cruz. MIT License.
 *  https://github.com/rstacruz/jquery-stuff/tree/master/scrollagent */

// Call $(...).scrollagent() with a callback function.
//
// The callback will be called everytime the focus changes.
//
// Example:
//
//      $("h2").scrollagent(function(cid, pid, currentElement, previousElement) {
//        if (pid) {
//          $("[href='#"+pid+"']").removeClass('active');
//        }
//        if (cid) {
//          $("[href='#"+cid+"']").addClass('active');
//        }
//      });

(function($) {

  $.fn.scrollagent = function(options, callback) {
    // Account for $.scrollspy(function)
    if (typeof callback === 'undefined') {
      callback = options;
      options = {};
    }

    var $sections = $(this);
    var $parent = options.parent || $(window);

    // Find the top offsets of each section
    var offsets = [];
    $sections.each(function(i) {
      var offset = $(this).attr('data-anchor-offset') ?
        parseInt($(this).attr('data-anchor-offset'), 10) :
        (options.offset || 0);

      offsets.push({
        id: $(this).attr('id'),
        index: i,
        el: this,
        offset: offset
      });
    });

    // State
    var current = null;
    var height = null;
    var range = null;

    // Save the height. Do this only whenever the window is resized so we don't
    // recalculate often.
    $(window).on('resize', function() {
      height = $parent.height();
      range = $(document).height();
    });

    // Find the current active section every scroll tick.
    $parent.on('scroll', function() {
      var y = $parent.scrollTop();
      // y += height * (0.3 + 0.7 * Math.pow(y/range, 2));

      var latest = null;

      for (var i in offsets) {
        if (offsets.hasOwnProperty(i)) {
          var offset = offsets[i];
          var el = offset.el;
          var relToViewport = offset.el.getBoundingClientRect().top;
          if(relToViewport > 0 && relToViewport < height / 2) {
            latest = offset; 
            break;
          }
        }
      }

      if (latest && (!current || (latest.index !== current.index))) {
        callback.call($sections,
          latest ? latest.id : null,
          current ? current.id : null,
          latest ? latest.el : null,
          current ? current.el : null);
        current = latest;
      }
    });

    $(window).trigger('resize');
    $parent.trigger('scroll');

    return this;
  };

})(jQuery);
/*! Anchorjump (c) 2012, Rico Sta. Cruz. MIT License.
 *   http://github.com/rstacruz/jquery-stuff/tree/master/anchorjump */

// Makes anchor jumps happen with smooth scrolling.
//
//    $("#menu a").anchorjump();
//    $("#menu a").anchorjump({ offset: -30 });
//
//    // Via delegate:
//    $("#menu").anchorjump({ for: 'a', offset: -30 });
//
// You may specify a parent. This makes it scroll down to the parent.
// Great for tabbed views.
//
//     $('#menu a').anchorjump({ parent: '.anchor' });
//
// You can jump to a given area.
//
//     $.anchorjump('#bank-deposit', options);

(function($) {
  var defaults = {
    'speed': 500,
    'offset': 0,
    'for': null,
    'parent': document.body
  };

  $.fn.anchorjump = function(options) {
    options = $.extend({}, defaults, options);

    if (options['for']) {
      this.on('click', options['for'], onClick);
    } else {
      this.on('click', onClick);
    }

    function onClick(e) {
      var $a = $(e.target).closest('a');
      if (e.ctrlKey || e.metaKey || e.altKey || $a.attr('target')) return;

      e.preventDefault();
      var href = $a.attr('href');

      $.anchorjump(href, options);
    }
  };

  // Jump to a given area.
  $.anchorjump = function(href, options) {
    options = $.extend({}, defaults, options);

    var top = 0;

    if (href != '#') {
      var $area = $(href);
      // Find the parent
      if (options.parent) {
        var $parent = $area.closest(options.parent);
        if ($parent.length) { $area = $parent; }
      }
      if (!$area.length) { return; }

      // Determine the pixel offset; use the default if not available
      var offset =
        $area.attr('data-anchor-offset') ?
        parseInt($area.attr('data-anchor-offset'), 10) :
        options.offset;

      // offset().top might be negative because the document is the thing that
      // overflow-scrolls.
      top = $area.offset().top + offset;
    }

    $('html, body').animate({ scrollTop: document.body.scrollTop + top }, options.speed);
    $('body').trigger('anchor', href);

    // Add the location hash via pushState.
    if (window.history.pushState) {
      window.history.pushState({ href: href }, "", href);
    }
  };
})(jQuery);
